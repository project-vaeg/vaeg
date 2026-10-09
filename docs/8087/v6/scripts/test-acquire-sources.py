#!/usr/bin/env python3
"""Offline helper tests. These do not test VAEG, chip semantics, OCR, or live websites."""
from __future__ import annotations
import copy
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import urllib.error
import zipfile

SCRIPT = Path(__file__).resolve().parent / 'acquire-sources.py'
spec = importlib.util.spec_from_file_location('vaeg_acquisition', SCRIPT)
assert spec and spec.loader
acq = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = acq
spec.loader.exec_module(acq)

PDF = b'%PDF-1.4\nHeader-check test fixture, not a structurally complete PDF.\n%%EOF\n'


def item(kind='pdf', action='FETCH_DOCUMENT', sid='I01'):
    return dict(id=sid, title='Synthetic reference', action=action, kind=kind, required=True,
                implementation_readable=action != 'FETCH_EVIDENCE_ONLY',
                ocr_candidate=kind == 'pdf' and action == 'FETCH_DOCUMENT',
                relative_path=f'originals/{sid}/fixture.{kind}',
                expected_sha256=None, max_bytes=100000,
                urls=[f'https://example.com/{sid}.{kind}'])


class Response(io.BytesIO):
    def __init__(self, data, url='https://example.com/I01.pdf', length=None, status=200):
        super().__init__(data)
        self.url = url
        self.status = status
        self.headers = {'Content-Length': str(len(data) if length is None else length),
                        'Content-Type': 'application/octet-stream'}

    def geturl(self):
        return self.url

    def getcode(self):
        return self.status


class Opener:
    def __init__(self, data=PDF, url='https://example.com/I01.pdf', **options):
        self.data, self.url, self.options = data, url, options
        self.calls = 0

    def open(self, request, timeout=30):
        self.calls += 1
        return Response(self.data, self.url, **self.options)


class FailingOpener:
    def __init__(self):
        self.calls = 0

    def open(self, request, timeout=30):
        self.calls += 1
        raise urllib.error.URLError('synthetic offline failure')


class Tests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name).resolve()

    def tearDown(self):
        self.temp.cleanup()

    def fetch(self, source=None, opener=None, budget=100000):
        return acq.fetch_item(source or item(), self.root, acq.Budget(budget),
                              opener=opener or Opener(), attempts=1)

    def test_01_pdf_download_receipt(self):
        result = self.fetch()
        self.assertEqual(result['status'], 'DOWNLOADED')
        self.assertEqual(result['sha256'], hashlib.sha256(PDF).hexdigest())
        self.assertFalse(result['edition_verified'])
        self.assertIn('NOT_FULL_STRUCTURE', result['validation'])

    def test_02_resume_without_network(self):
        self.fetch()
        opener = FailingOpener()
        self.assertEqual(self.fetch(opener=opener)['status'], 'REUSED_VERIFIED')
        self.assertEqual(opener.calls, 0)

    def test_03_existing_hash_conflict(self):
        self.fetch()
        path = self.root/item()['relative_path']
        path.write_bytes(PDF+b'mutated')
        self.assertEqual(self.fetch()['status'], 'EXISTING_FILE_CONFLICT')
        self.assertEqual(path.read_bytes(), PDF+b'mutated')

    def test_04_unmanaged_existing_original(self):
        path = self.root/item()['relative_path']
        path.parent.mkdir(parents=True); path.write_bytes(PDF)
        self.assertEqual(self.fetch()['status'], 'EXISTING_FILE_CONFLICT')
        self.assertEqual(path.read_bytes(), PDF)

    def test_05_html_is_not_pdf(self):
        self.assertEqual(self.fetch(opener=Opener(b'<html>error</html>'))['status'], 'FAILED')
        self.assertFalse((self.root/item()['relative_path']).exists())

    def test_06_truncated_pdf(self):
        self.assertEqual(self.fetch(opener=Opener(b'%PDF-1.4\ntruncated'))['status'], 'FAILED')

    def test_07_content_length_mismatch(self):
        self.assertEqual(self.fetch(opener=Opener(length=len(PDF)+1))['status'], 'FAILED')

    def test_08_expected_hash_mismatch(self):
        source=item(); source['expected_sha256']='0'*64
        self.assertEqual(self.fetch(source)['status'], 'FAILED')
        self.assertFalse((self.root/source['relative_path']).exists())

    def test_09_source_limit(self):
        source=item(); source['max_bytes']=1
        self.assertEqual(self.fetch(source)['status'], 'FAILED')

    def test_10_total_limit(self):
        self.assertEqual(self.fetch(budget=1)['status'], 'FAILED')

    def test_11_network_failure(self):
        opener=FailingOpener()
        self.assertEqual(self.fetch(opener=opener)['status'], 'FAILED')
        self.assertEqual(opener.calls, 1)

    def test_12_redirect_not_allowlisted(self):
        self.assertEqual(self.fetch(opener=Opener(url='https://other.example/I01.pdf'))['status'], 'FAILED')

    def test_13_http_downgrade(self):
        self.assertEqual(self.fetch(opener=Opener(url='http://example.com/I01.pdf'))['status'], 'FAILED')

    def test_14_private_url(self):
        for url in ['https://127.0.0.1/a','https://localhost/a','https://user:password@example.com/a']:
            with self.assertRaises(acq.AcquisitionError):
                acq.public_https(url)

    def test_15_path_traversal(self):
        for rel in ['../x', '/x', 'a/../b', 'a//b', 'a\\b', 'C:/x']:
            with self.assertRaises(acq.AcquisitionError): acq.safe_relative(rel)

    def test_16_zip_crc_without_extraction(self):
        buffer=io.BytesIO()
        with zipfile.ZipFile(buffer, 'w', zipfile.ZIP_DEFLATED) as archive:
            archive.writestr('fixture/README.txt','No executable content.')
        source=item('zip','FETCH_ARCHIVE')
        result=self.fetch(source, Opener(buffer.getvalue(),url=source['urls'][0]))
        self.assertEqual(result['status'],'DOWNLOADED')
        self.assertFalse((self.root/'fixture').exists())
        self.assertIn('NOT_EXTRACTED',result['validation'])

    def test_17_zip_traversal_rejected(self):
        buffer=io.BytesIO()
        with zipfile.ZipFile(buffer, 'w') as archive: archive.writestr('../bad','bad')
        source=item('zip','FETCH_ARCHIVE')
        self.assertEqual(self.fetch(source,Opener(buffer.getvalue(),url=source['urls'][0]))['status'],'FAILED')

    def test_18_html_identity(self):
        source=item('html'); source['html_identity_tokens']=['SoftFloat']
        result=self.fetch(source,Opener(b'<html>SoftFloat documentation</html>',url=source['urls'][0]))
        self.assertEqual(result['status'],'DOWNLOADED')

    def test_19_html_challenge(self):
        source=item('html'); source['html_identity_tokens']=['SoftFloat']
        result=self.fetch(source,Opener(b'<html><title>Access Denied</title>SoftFloat</html>',url=source['urls'][0]))
        self.assertEqual(result['status'],'FAILED')

    def test_20_metadata_never_fetches(self):
        source=item();source.update(action='METADATA_ONLY',kind='none',required=False,
                                    ocr_candidate=False,implementation_readable=False,
                                    relative_path=None,urls=[])
        opener=FailingOpener()
        self.assertEqual(self.fetch(source,opener)['status'],'METADATA_ONLY')
        self.assertEqual(opener.calls,0)

    def test_21_reports_and_user_stop(self):
        result=self.fetch()
        run=acq.write_reports(self.root,{},'a'*64,[result],acq.Budget(1000))
        report=json.loads((run/'download-report.json').read_text())
        self.assertEqual(report['workflow_state'],'AWAITING_USER_OCR')
        self.assertFalse(report['implementation_authorized'])
        self.assertFalse(report['ocr_performed'])
        self.assertEqual(len((run/'ocr-queue.tsv').read_text().splitlines()),2)
        self.assertFalse((self.root/'user-ocr/I01/document.md').exists())

    def test_22_incomplete_handoff(self):
        result=self.fetch(opener=FailingOpener())
        run=acq.write_reports(self.root,{},'a'*64,[result],acq.Budget(1000))
        report=json.loads((run/'download-report.json').read_text())
        self.assertEqual(report['workflow_state'],'ACQUISITION_INCOMPLETE_AWAITING_USER')
        self.assertEqual(report['required_missing'],['I01'])

    def test_23_evidence_only_not_in_ocr_queue(self):
        source=item('html','FETCH_EVIDENCE_ONLY','H01')
        source['relative_path']='evidence-only/H01/article.html'
        result=self.fetch(source,Opener(b'<html>Example article</html>',url=source['urls'][0]))
        run=acq.write_reports(self.root,{},'a'*64,[result],acq.Budget(1000))
        self.assertEqual(len((run/'ocr-queue.tsv').read_text().splitlines()),1)

    def test_24_preserve_user_ocr_and_prior_runs(self):
        result=self.fetch()
        run1=acq.write_reports(self.root,{},'a'*64,[result],acq.Budget(1000))
        ocr=self.root/'user-ocr/I01/document.md';ocr.write_text('User text')
        run2=acq.write_reports(self.root,{},'a'*64,[result],acq.Budget(1000))
        self.assertNotEqual(run1,run2)
        self.assertTrue((run1/'download-report.json').exists())
        self.assertEqual(ocr.read_text(),'User text')

    def test_25_actual_catalog(self):
        catalog=acq.validate_catalog(SCRIPT.parent.parent/'sources/catalog.json')
        self.assertEqual(len(catalog['sources']),23)
        self.assertEqual(sum(e['action'] in acq.FETCH_ACTIONS for e in catalog['sources']),16)
        self.assertEqual(sum(e['ocr_candidate'] for e in catalog['sources']),7)

    def test_26_duplicate_catalog_entry(self):
        catalog=acq.validate_catalog(SCRIPT.parent.parent/'sources/catalog.json')
        catalog['sources'].append(copy.deepcopy(catalog['sources'][0]))
        path=self.root/'bad.json';path.write_text(json.dumps(catalog))
        with self.assertRaises(acq.AcquisitionError):acq.validate_catalog(path)

    def test_27_git_contained_evidence_rejected(self):
        repo=self.root/'repo';repo.mkdir()
        subprocess.run(['git','init','-q',str(repo)],check=True)
        with self.assertRaises(acq.AcquisitionError):acq.evidence_root(str(repo/'cache'),repo)
        self.assertEqual(acq.evidence_root(str(self.root/'cache'),repo),self.root/'cache')

    def test_28_preview_makes_no_workspace(self):
        repo=self.root/'repo';repo.mkdir()
        subprocess.run(['git','init','-q',str(repo)],check=True)
        cache=self.root/'cache'
        result=subprocess.run([sys.executable,str(SCRIPT),'--repo',str(repo),'--evidence-root',str(cache)],
                              text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertFalse(cache.exists())

    def test_29_symlink_rejected(self):
        real=self.root/'real';real.mkdir()
        link=self.root/'link'
        try:link.symlink_to(real,target_is_directory=True)
        except OSError:self.skipTest('Symlink creation is not supported in this test environment')
        with self.assertRaises(acq.AcquisitionError):acq.reject_symlinks(link/'x')

    def test_30_unmanaged_latest_not_overwritten(self):
        latest=self.root/'latest.json';latest.write_text('{"owner":"someone-else"}')
        with self.assertRaises(acq.AcquisitionError):
            acq.write_reports(self.root,{},'a'*64,[self.fetch()],acq.Budget(1000))
        self.assertEqual(json.loads(latest.read_text())['owner'],'someone-else')


if __name__ == '__main__':
    unittest.main(verbosity=2)
