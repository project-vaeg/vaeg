#!/usr/bin/env python3
"""Acquire VAEG 8087 references without OCR, extraction, builds, or implementation.

Default mode previews only. Use --apply for bounded network downloads. All fetched
bytes, receipts and handoff reports stay in an external, non-Git workspace.
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass
from datetime import datetime, timezone
import hashlib
import http.client
import ipaddress
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import subprocess
import sys
import tempfile
import time
from typing import Any
import urllib.error
import urllib.parse
import urllib.request
import uuid
import zipfile

OWNER = 'VAEG8087_ACQUISITION_V6'
FETCH_ACTIONS = {'FETCH_DOCUMENT', 'FETCH_ARCHIVE', 'FETCH_EVIDENCE_ONLY'}
ALL_ACTIONS = FETCH_ACTIONS | {'METADATA_ONLY', 'DISCOVERY_ONLY'}
SUCCESS = {'DOWNLOADED', 'REUSED_VERIFIED'}


class AcquisitionError(Exception):
    """A safe acquisition or input-validation failure."""


class BudgetExceeded(AcquisitionError):
    """The invocation's transfer limit has been reached."""


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec='seconds')


def file_hash(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def safe_relative(value: Any) -> PurePosixPath:
    if not isinstance(value, str) or not value or '\\' in value or ':' in value:
        raise AcquisitionError('Invalid relative path')
    path = PurePosixPath(value)
    if path.is_absolute() or any(part in ('.', '..') for part in path.parts):
        raise AcquisitionError(f'Unsafe relative path: {value}')
    if path.as_posix() != value:
        raise AcquisitionError(f'Non-normalized relative path: {value}')
    return path


def reject_symlinks(path: Path) -> None:
    current = Path(path.anchor)
    for part in path.parts[1:]:
        current /= part
        if current.is_symlink():
            raise AcquisitionError(f'Symlink path component is not allowed: {current}')


def contained_path(root: Path, relative: str) -> Path:
    path = root.joinpath(*safe_relative(relative).parts)
    reject_symlinks(path)
    return path


def git_result(path: Path, *args: str) -> subprocess.CompletedProcess[str]:
    env = {k: v for k, v in os.environ.items() if not k.startswith('GIT_')}
    return subprocess.run(['git', '-C', str(path), *args], text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          env=env, check=False)


def repository_root(value: str) -> Path:
    path = Path(value).expanduser().resolve(strict=True)
    result = git_result(path, 'rev-parse', '--show-toplevel')
    if result.returncode or Path(result.stdout.strip()).resolve() != path:
        raise AcquisitionError('Use the existing Git worktree root with --repo')
    return path


def evidence_root(value: str | None, repo: Path) -> Path:
    path = Path(value).expanduser() if value else repo.parent / 'vaeg-8087-evidence-v6'
    path = Path(os.path.abspath(path))
    reject_symlinks(path)
    if path == repo or repo in path.parents:
        raise AcquisitionError('Evidence must be outside the repository')
    for ancestor in (path, *path.parents):
        if (ancestor / '.git').exists() or (ancestor / '.git').is_symlink():
            raise AcquisitionError(f'Evidence would be inside a Git worktree: {ancestor}')
    existing = path
    while not existing.exists():
        existing = existing.parent
    if not existing.is_dir():
        raise AcquisitionError('Evidence root has a non-directory ancestor')
    if git_result(existing, 'rev-parse', '--git-dir').returncode == 0:
        raise AcquisitionError('Evidence must be outside all Git repositories')
    if path.exists() and not path.is_dir():
        raise AcquisitionError('Evidence root is not a directory')
    return path


def public_https(url: str, allowed_hosts: set[str] | None = None) -> str:
    parts = urllib.parse.urlsplit(url)
    host = (parts.hostname or '').lower()
    if parts.scheme != 'https' or not host or parts.username or parts.password or parts.fragment:
        raise AcquisitionError('Only credential-free HTTPS URLs without fragments are allowed')
    if parts.port not in (None, 443) or host == 'localhost' or host.endswith('.local'):
        raise AcquisitionError('Non-public URL or unexpected port')
    try:
        address = ipaddress.ip_address(host)
    except ValueError:
        address = None
    if address is not None and not address.is_global:
        raise AcquisitionError('Private or loopback address is not allowed')
    if allowed_hosts is not None and host not in allowed_hosts:
        raise AcquisitionError(f'Redirect host is not in this source allowlist: {host}')
    return host


def validate_catalog(path: Path) -> dict[str, Any]:
    reject_symlinks(path)
    catalog = json.loads(path.read_text(encoding='utf-8'))
    if catalog.get('format_version') != 1 or catalog.get('pack_version') != 'v6':
        raise AcquisitionError('Unsupported catalog format')
    entries = catalog.get('sources')
    if not isinstance(entries, list) or not entries:
        raise AcquisitionError('Catalog has no sources')
    seen_ids: set[str] = set()
    seen_paths: set[str] = set()
    for item in entries:
        sid = item.get('id', '')
        if not re.fullmatch(r'[A-Z][A-Z0-9_-]{1,31}', sid) or sid in seen_ids:
            raise AcquisitionError(f'Invalid or duplicate source ID: {sid}')
        seen_ids.add(sid)
        if item.get('action') not in ALL_ACTIONS or not isinstance(item.get('required'), bool):
            raise AcquisitionError(f'Invalid action or requirement flag: {sid}')
        pin = item.get('expected_sha256')
        if pin is not None and not re.fullmatch(r'[0-9a-f]{64}', pin):
            raise AcquisitionError(f'Invalid expected SHA-256: {sid}')
        if item['action'] not in FETCH_ACTIONS:
            if item['required']:
                raise AcquisitionError('Metadata/discovery entries cannot be mandatory downloads')
            continue
        if item.get('kind') not in ('pdf', 'html', 'zip'):
            raise AcquisitionError(f'Unsupported content kind: {sid}')
        rel = safe_relative(item.get('relative_path')).as_posix()
        if rel in seen_paths or rel.split('/')[0] not in ('originals', 'upstream', 'evidence-only'):
            raise AcquisitionError(f'Invalid or duplicate output path: {rel}')
        seen_paths.add(rel)
        if item['action'] == 'FETCH_EVIDENCE_ONLY':
            if not rel.startswith('evidence-only/') or item.get('implementation_readable'):
                raise AcquisitionError('Evidence-only input must remain segregated')
        elif not item.get('implementation_readable'):
            raise AcquisitionError('Document/archive action requires an explicit readable flag')
        if item.get('ocr_candidate') != (item['kind'] == 'pdf' and item['action'] == 'FETCH_DOCUMENT'):
            raise AcquisitionError(f'Invalid OCR queue classification: {sid}')
        urls = item.get('urls')
        if not isinstance(urls, list) or not urls or len(urls) > 4:
            raise AcquisitionError(f'Invalid URL list: {sid}')
        for url in urls:
            public_https(url)
        for host in item.get('allowed_redirect_hosts', []):
            if public_https('https://' + host + '/') != host:
                raise AcquisitionError(f'Invalid redirect host: {host}')
        if not 1 <= item.get('max_bytes', 268435456) <= 268435456:
            raise AcquisitionError(f'Invalid byte limit: {sid}')
    return catalog


class SafeRedirect(urllib.request.HTTPRedirectHandler):
    def __init__(self, hosts: set[str]):
        super().__init__()
        self.hosts = hosts

    def redirect_request(self, req, fp, code, msg, headers, newurl):
        public_https(newurl, self.hosts)
        return super().redirect_request(req, fp, code, msg, headers, newurl)


def make_opener(item: dict[str, Any]):
    hosts = {public_https(url) for url in item['urls']}
    hosts.update(item.get('allowed_redirect_hosts', []))
    return urllib.request.build_opener(SafeRedirect(hosts))


@dataclass
class Budget:
    maximum: int
    used: int = 0

    def add(self, count: int) -> None:
        self.used += count
        if self.used > self.maximum:
            raise BudgetExceeded('Total transfer byte limit exceeded')


def inspect_bytes(path: Path, item: dict[str, Any]) -> str:
    """Check containers only; never OCR, extract manual text, or unpack an archive."""
    size = path.stat().st_size
    if size <= 0 or size > item.get('max_bytes', 268435456):
        raise AcquisitionError('File is empty or exceeds its limit')
    with path.open('rb') as stream:
        head = stream.read(4096)
        if item['kind'] == 'pdf':
            if b'%PDF-' not in head[:1024]:
                raise AcquisitionError('Not a PDF header; possible HTML error page')
            stream.seek(max(0, size - 65536))
            if b'%%EOF' not in stream.read():
                raise AcquisitionError('Missing PDF EOF marker; possible truncated download')
            return 'PDF_HEADER_AND_EOF_ONLY_NOT_FULL_STRUCTURE_OR_EDITION_CHECK'
        if item['kind'] == 'html':
            if b'<html' not in head.lower() and b'<!doctype html' not in head.lower():
                raise AcquisitionError('Not recognizable HTML')
            stream.seek(0)
            body = stream.read(min(size, 16 * 1024 * 1024)).lower()
            if any(x in body for x in (b'cf-chl-', b'<title>access denied', b'<title>403 forbidden')):
                raise AcquisitionError('Access-denied or browser-challenge HTML')
            for token in item.get('html_identity_tokens', []):
                if token.lower().encode('utf-8') not in body:
                    raise AcquisitionError(f'Missing HTML identity token: {token}')
            return 'HTML_CONTAINER_AND_IDENTITY_TOKENS_ONLY'
    if item['kind'] == 'zip':
        with zipfile.ZipFile(path) as archive:
            infos = archive.infolist()
            if not infos or len(infos) > 20000 or sum(i.file_size for i in infos) > 128 * 1024 * 1024:
                raise AcquisitionError('ZIP verification budget exceeded or empty archive')
            for info in infos:
                if info.flag_bits & 1:
                    raise AcquisitionError('Encrypted ZIP is not supported')
                if info.compress_type not in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED):
                    raise AcquisitionError('Unsupported ZIP compression for bounded verification')
                safe_relative(info.filename.rstrip('/'))
                if (info.external_attr >> 16) & 0o170000 == 0o120000:
                    raise AcquisitionError('ZIP contains a symbolic link')
            bad = archive.testzip()
            if bad:
                raise AcquisitionError(f'ZIP CRC failure: {bad}')
        return 'ZIP_DIRECTORY_AND_CRC_CHECKED_NOT_EXTRACTED'
    raise AcquisitionError('Unsupported content kind')


def exclusive_json(path: Path, value: Any) -> None:
    reject_symlinks(path)
    with path.open('x', encoding='utf-8') as stream:
        json.dump(value, stream, indent=2, ensure_ascii=False)
        stream.write('\n')


def existing_receipt(target: Path, item: dict[str, Any]) -> dict[str, Any] | None:
    receipt_path = Path(str(target) + '.receipt.json')
    reject_symlinks(receipt_path)
    if not target.exists() and not receipt_path.exists():
        return None
    if not target.is_file() or not receipt_path.is_file():
        raise AcquisitionError('Existing file/receipt pair is incomplete; will not overwrite')
    receipt = json.loads(receipt_path.read_text(encoding='utf-8'))
    actual = file_hash(target)
    if (receipt.get('owner') != OWNER or receipt.get('id') != item['id'] or
            receipt.get('action') != item['action'] or receipt.get('sha256') != actual or
            receipt.get('bytes') != target.stat().st_size):
        raise AcquisitionError('Existing receipt or file hash conflicts; will not overwrite')
    if item.get('expected_sha256') not in (None, actual):
        raise AcquisitionError('Existing file conflicts with the expected SHA-256')
    receipt['validation'] = inspect_bytes(target, item)
    receipt['status'] = 'REUSED_VERIFIED'
    receipt['checked_at'] = utc_now()
    return receipt


def fetch_item(item: dict[str, Any], root: Path, budget: Budget,
               opener=None, attempts: int = 2, timeout: int = 30,
               deadline: int = 180) -> dict[str, Any]:
    result = {k: item[k] for k in ('id', 'title', 'action', 'kind', 'required',
                                 'implementation_readable', 'ocr_candidate')}
    result.update(relative_path=item.get('relative_path'), expected_sha256=item.get('expected_sha256'))
    if item['action'] not in FETCH_ACTIONS:
        result['status'] = 'DISCOVERY_REQUIRED' if item['action'] == 'DISCOVERY_ONLY' else 'METADATA_ONLY'
        return result
    target = contained_path(root, item['relative_path'])
    try:
        cached = existing_receipt(target, item)
        if cached:
            result.update(cached)
            # Current policy flags, not old receipts, determine queue inclusion.
            result.update(id=item['id'], title=item['title'], kind=item['kind'], action=item['action'],
                          relative_path=item['relative_path'], expected_sha256=item.get('expected_sha256'),
                          required=item['required'], ocr_candidate=item['ocr_candidate'],
                          implementation_readable=item['implementation_readable'])
            return result
    except (AcquisitionError, OSError, ValueError, http.client.HTTPException, zipfile.BadZipFile) as error:
        result.update(status='EXISTING_FILE_CONFLICT', errors=[str(error)])
        return result
    target.parent.mkdir(parents=True, exist_ok=True)
    reject_symlinks(target.parent)
    attempts_log = []
    opener = opener or make_opener(item)
    allowed_hosts = {public_https(url) for url in item['urls']} | set(item.get('allowed_redirect_hosts', []))
    for url in item['urls']:
        for attempt in range(1, attempts + 1):
            temp_path = None
            try:
                if budget.used >= budget.maximum:
                    raise BudgetExceeded('Total transfer byte limit reached')
                start = time.monotonic()
                request = urllib.request.Request(url, headers={
                    'User-Agent': 'VAEG-8087-reference-fetch/6', 'Accept-Encoding': 'identity'})
                with opener.open(request, timeout=timeout) as response:
                    final_url = response.geturl()
                    public_https(final_url, allowed_hosts)
                    if response.getcode() != 200:
                        raise AcquisitionError(f'Unexpected HTTP status: {response.getcode()}')
                    length = response.headers.get('Content-Length')
                    if length is not None and (not length.isdigit() or int(length) > item['max_bytes']):
                        raise AcquisitionError('Invalid or oversized Content-Length')
                    content_type = response.headers.get('Content-Type', '')
                    with tempfile.NamedTemporaryFile(dir=target.parent, prefix='.fetch-',
                                                     suffix='.part', delete=False) as output:
                        temp_path = Path(output.name)
                        count = 0
                        while True:
                            if time.monotonic() - start > deadline:
                                raise AcquisitionError('Total attempt deadline exceeded')
                            chunk = response.read(256 * 1024)
                            if not chunk:
                                break
                            budget.add(len(chunk)); count += len(chunk)
                            if count > item['max_bytes']:
                                raise AcquisitionError('Source byte limit exceeded')
                            output.write(chunk)
                        if time.monotonic() - start > deadline:
                            raise AcquisitionError('Total attempt deadline exceeded')
                    if length is not None and count != int(length):
                        raise AcquisitionError('Truncated or inconsistent Content-Length')
                validation = inspect_bytes(temp_path, item)
                actual = file_hash(temp_path)
                if item.get('expected_sha256') not in (None, actual):
                    raise AcquisitionError('Downloaded file does not match expected SHA-256')
                receipt = dict(result, owner=OWNER, status='DOWNLOADED', requested_url=url,
                               final_url=final_url, retrieved_at=utc_now(), bytes=count,
                               sha256=actual, content_type=content_type, validation=validation,
                               edition_verified=False,
                               hash_authentication='OBSERVED_HASH_NOT_PUBLISHER_SIGNATURE',
                               attempts=attempts_log + [dict(url=url, attempt=attempt, status='SUCCESS')])
                # Exclusive creation is portable and never replaces existing user files.
                with temp_path.open('rb') as inp, target.open('xb') as out:
                    shutil.copyfileobj(inp, out, 1024 * 1024)
                if file_hash(target) != actual:
                    raise AcquisitionError('Published file hash mismatch')
                exclusive_json(Path(str(target) + '.receipt.json'), receipt)
                return receipt
            except (AcquisitionError, OSError, ValueError, http.client.HTTPException, zipfile.BadZipFile) as error:
                attempts_log.append(dict(url=url, attempt=attempt, status='FAILED', error=str(error)))
                if isinstance(error, BudgetExceeded) or target.exists():
                    result.update(status='FAILED', attempts=attempts_log)
                    return result
                if attempt < attempts:
                    time.sleep(1)
            finally:
                if temp_path is not None and temp_path.exists():
                    temp_path.unlink()  # Only this attempt's own temporary file.
    result.update(status='FAILED', attempts=attempts_log)
    return result


def write_reports(root: Path, catalog: dict[str, Any], catalog_hash: str,
                  results: list[dict[str, Any]], budget: Budget, interrupted: bool = False) -> Path:
    run_id = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ') + '-' + uuid.uuid4().hex[:8]
    run = contained_path(root, 'runs/' + run_id)
    run.mkdir(parents=True, exist_ok=False)
    required_missing = [r['id'] for r in results if r['required'] and r['status'] not in SUCCESS]
    state = 'ACQUISITION_INCOMPLETE_AWAITING_USER' if required_missing or interrupted else 'AWAITING_USER_OCR'
    report = dict(owner=OWNER, format_version=1, run_id=run_id, created_at=utc_now(),
                  workflow_state=state, stage='A', implementation_authorized=False,
                  emulator_status='NOT_VERIFIED_BY_ACQUISITION', ocr_performed=False,
                  interrupted=interrupted, catalog_sha256=catalog_hash,
                  transferred_bytes=budget.used, required_missing=required_missing,
                  catalog_snapshot=catalog, results=results)
    exclusive_json(run/'download-report.json', report)
    successful = [r for r in results if r['status'] in SUCCESS]
    queue = [r for r in successful if r['ocr_candidate'] and r['implementation_readable']]
    summary = ['# Reference acquisition report', '', f'State: **{state}**.',
               'No OCR, archive extraction, build, vendoring or emulator implementation was performed.',
               f'Catalog SHA-256: `{catalog_hash}`.', '',
               '| ID | Status | Bytes | External relative path |', '|---|---|---:|---|']
    for row in results:
        summary.append(f"| {row['id']} | {row['status']} | {row.get('bytes', '')} | {row.get('relative_path') or '-'} |")
    summary += ['', 'Required missing: ' + (', '.join(required_missing) or 'none'), '',
                'See download-report.json for URL attempts, hashes and errors. PDF checks are header/EOF',
                'checks only, not full structural, edition, table or semantic verification.',
                'Evidence-only articles are not implementation inputs. Acquisition is not emulator QA.', '']
    (run/'download-report.md').write_text('\n'.join(summary), encoding='utf-8')
    (run/'CHECKSUMS.sha256').write_text(''.join(f"{r['sha256']}  {r['relative_path']}\n" for r in successful), encoding='utf-8')
    fields = ['source_id', 'priority', 'original_relative_path', 'original_sha256', 'suggested_ocr_directory']
    rows = ['\t'.join(fields)]
    for row in queue:
        ocr_dir = 'user-ocr/' + row['id']
        contained_path(root, ocr_dir).mkdir(parents=True, exist_ok=True)
        rows.append('\t'.join([row['id'], 'CORE' if row['required'] else 'OPTIONAL',
                               row['relative_path'], row['sha256'], ocr_dir]))
    (run/'ocr-queue.tsv').write_text('\n'.join(rows)+'\n', encoding='utf-8')
    handoff = f'''# OCR handoff - Stage A ends here

State: **{state}**. Next actor: **USER**.

Evidence root: `{root}`
Report directory: `{run}`
OCR queue: `{run / 'ocr-queue.tsv'}`
Checksums: `{run / 'CHECKSUMS.sha256'}`
Download details: `{run / 'download-report.json'}`

Successful files: {len(successful)}. PDF OCR queue: {len(queue)}.
Required missing: {', '.join(required_missing) or 'none'}.

The user performs OCR. Keep originals unchanged. Place Markdown under
`{root / 'user-ocr'}/<source-id>/document.md` (split Markdown is acceptable).
Prefer 1-based PDF page markers and separate printed page labels. Do not overwrite scans.
There are no generated OCR-content placeholders and no implementation authorization.

Consult docs/8087/v6/ocr-handoff.md in the VAEG worktree for the acceptance rules.
After OCR, explicitly authorize Stage B with activation-implement.txt and identify the
actual OCR directory. Existing text layers or new files do not trigger implementation.

No agent OCR, archive extraction, dependency build, vendoring or P00-P19 work is permitted
before that subsequent user instruction. Optional/reference-only failures do not disappear;
all actual attempts remain in the JSON report. Header/EOF checks are not edition validation.
'''
    (run/'OCR-HANDOFF.md').write_text(handoff, encoding='utf-8')
    latest = contained_path(root, 'latest.json')
    if latest.exists():
        old = json.loads(latest.read_text(encoding='utf-8'))
        if old.get('owner') != OWNER:
            raise AcquisitionError('Refusing to replace an unmanaged latest.json')
    temp = root / ('.latest-' + uuid.uuid4().hex + '.json')
    exclusive_json(temp, dict(owner=OWNER, run_id=run_id, workflow_state=state,
                             report_directory='runs/' + run_id,
                             handoff='runs/' + run_id + '/OCR-HANDOFF.md',
                             implementation_authorized=False))
    os.replace(temp, latest)
    return run


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', required=True, help='Existing VAEG Git worktree root')
    parser.add_argument('--evidence-root', help='Explicit directory outside all Git repositories')
    parser.add_argument('--catalog', type=Path, default=Path(__file__).resolve().parents[1]/'sources/catalog.json')
    parser.add_argument('--apply', action='store_true', help='Acquire; otherwise preview only')
    args = parser.parse_args()
    lock = None
    owns_lock = False
    try:
        repo = repository_root(args.repo)
        root = evidence_root(args.evidence_root, repo)
        catalog_path = args.catalog.expanduser().resolve(strict=True)
        catalog = validate_catalog(catalog_path)
        print(f'Evidence workspace: {root}')
        for item in catalog['sources']:
            print(f"{item['id']}: {item['action']} -> {item.get('relative_path') or '(no download)'}")
        if not args.apply:
            print('Preview only. No network requests or filesystem changes. Re-run with --apply.')
            return 0
        root.mkdir(parents=True, exist_ok=True)
        reject_symlinks(root)
        lock = root / '.acquisition.lock'
        with lock.open('x', encoding='utf-8') as stream:
            stream.write(f'pid={os.getpid()} created_at={utc_now()}\n')
        owns_lock = True
        marker = contained_path(root, '.workspace.json')
        if marker.exists():
            if json.loads(marker.read_text(encoding='utf-8')).get('owner') != OWNER:
                raise AcquisitionError('Evidence workspace marker belongs to another tool')
        else:
            exclusive_json(marker, dict(owner=OWNER, created_at=utc_now()))
        defaults = catalog.get('defaults', {})
        budget = Budget(min(int(defaults.get('max_total_transfer_bytes', 536870912)), 536870912))
        if budget.maximum < 1:
            raise AcquisitionError('Invalid total transfer limit')
        results = []
        interrupted = False
        for item in catalog['sources']:
            try:
                result = fetch_item(item, root, budget,
                                    attempts=min(2, max(1, int(defaults.get('max_attempts_per_url', 2)))),
                                    timeout=min(30, max(1, int(defaults.get('inactivity_timeout_seconds', 30)))),
                                    deadline=min(180, max(1, int(defaults.get('attempt_deadline_seconds', 180)))))
                results.append(result)
                print(f"{item['id']}: {result['status']}")
            except KeyboardInterrupt:
                interrupted = True
                break
        known = {r['id'] for r in results}
        for item in catalog['sources']:
            if item['id'] not in known:
                results.append(dict(id=item['id'], title=item['title'], action=item['action'],
                                    kind=item['kind'], required=item['required'], status='NOT_ATTEMPTED',
                                    relative_path=item.get('relative_path'),
                                    implementation_readable=item.get('implementation_readable', False),
                                    ocr_candidate=item.get('ocr_candidate', False)))
        run = write_reports(root, catalog, file_hash(catalog_path), results, budget, interrupted)
        print(f'Handoff: {run / "OCR-HANDOFF.md"}')
        print('STOP. The user performs OCR. No implementation is authorized by this command.')
        if interrupted:
            return 130
        return 3 if any(r['required'] and r['status'] not in SUCCESS for r in results) else 0
    except (AcquisitionError, OSError, ValueError, TypeError, KeyError) as error:
        print(f'ERROR: {error}', file=sys.stderr)
        print('No OCR or implementation was authorized. Preserve existing files and inspect the error.', file=sys.stderr)
        return 2
    finally:
        if owns_lock and lock is not None:
            lock.unlink(missing_ok=True)  # Only the lock created by this invocation.


if __name__ == '__main__':
    raise SystemExit(main())
