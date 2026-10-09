#!/usr/bin/env python3
"""Materialize campaign-local M65 execution specifications."""
# Copyright (c) 2026 Nakata Maho
import hashlib
import json
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[2]
BASE = "efd96b7e46717e7ee56e086f7d27ba42b04b49d3"

def digest(values):
    data = json.dumps(sorted(values), sort_keys=True, separators=(",", ":"))
    return hashlib.sha256(data.encode()).hexdigest()

def main():
    inv = json.loads((ROOT / "tests/ssts/evidence/g65/architectural_residue.json").read_text())
    rows = inv["rows"]
    forms = {"M65a":"FF.7", "M65b":"62", "M65c":"F7.2", "M65d":"FF.6"}
    specs = []
    for task, form in forms.items():
        selected = [r for r in rows if r["form"] == form]
        hashes = [r["record_hash"] for r in selected]
        specs.append({"task_id":task,"canonical_owner":task,"task_kind":"applicable_semantic_failure","readiness_status":"evidence_blocked","campaign_base_gate":"G65","campaign_base_sha":BASE,"owned_selectors":[form],"owned_hash_count":len(hashes),"owned_hash_set_sha256":digest(hashes),"owned_hash_artifact":"tests/ssts/evidence/g65/architectural_residue.json","current_classification":"applicable","current_applicable_state":"executed","evidence_sources":["G65 architectural residue scoreboard"],"case_table_path":None,"case_table_sha256":None,"structural_partitions":["form","mismatch_classes","termination_classes"],"proven_contract":False,"underdetermined_questions":["complete expected/actual architectural state is absent from committed G65 evidence"],"allowed_semantic_scope":"canonical task scope only","prohibited_scope":["implementation before complete expected/actual table","policy or fixture changes"],"required_profiles":["architectural CI","architectural full"],"stop_conditions":["missing expected/actual case rows"]})
    tail = [r for r in rows if r["form"] not in forms.values()]
    hashes = [r["record_hash"] for r in tail]
    specs.append({"task_id":"M65e","canonical_owner":"M65e","task_kind":"applicable_semantic_failure","readiness_status":"evidence_blocked","campaign_base_gate":"G65","campaign_base_sha":BASE,"owned_selectors":sorted({r["form"] for r in tail}),"owned_hash_count":len(hashes),"owned_hash_set_sha256":digest(hashes),"owned_hash_artifact":"tests/ssts/evidence/g65/architectural_residue.json","current_classification":"applicable","current_applicable_state":"executed","evidence_sources":["G65 architectural residue scoreboard"],"case_table_path":None,"case_table_sha256":None,"structural_partitions":["individual form","mismatch_classes","termination_classes"],"proven_contract":False,"underdetermined_questions":["complete expected/actual architectural state is absent from committed G65 evidence"],"allowed_semantic_scope":"exact ten cases only","prohibited_scope":["generic tail implementation","policy or fixture changes"],"required_profiles":["architectural CI","architectural full"],"stop_conditions":["missing expected/actual case rows"]})
    for task, status in {"M65f":"conditional_nonblocking","M65g":"conditional_nonblocking","M65h":"evidence_blocked","M65i":"conditional_nonblocking","M65j":"conditional_nonblocking","M65k":"closure_only","M65l":"conditional_nonblocking","M65m":"closure_only"}.items():
        specs.append({"task_id":task,"canonical_owner":task,"task_kind":"evidence_or_closure","readiness_status":status,"campaign_base_gate":"G65","campaign_base_sha":BASE,"owned_selectors":[],"owned_hash_count":0,"owned_hash_set_sha256":digest([]),"proven_contract":False,"evidence_sources":["canonical task and G65 manifests"],"case_table_path":None,"case_table_sha256":None,"allowed_semantic_scope":"none during materialization","prohibited_scope":["CPU semantics","policy changes","fixture changes"],"stop_conditions":["canonical evidence contract unavailable"]})
    out = ROOT / "tests/ssts/campaigns/g65m/execution_specs"; out.mkdir(parents=True, exist_ok=True)
    docs = ROOT / "docs/agents/campaigns/g65m/execution_specs"; docs.mkdir(parents=True, exist_ok=True)
    for spec in specs:
        (out / f"{spec['task_id'].lower()}.json").write_text(json.dumps(spec, sort_keys=True, indent=2) + "\n")
        (docs / f"{spec['task_id'].lower()}.md").write_text(f"# {spec['task_id']} execution specification\n\nReadiness: `{spec['readiness_status']}`\n\nOwned count: `{spec['owned_hash_count']}`\n\nOwned hash digest: `{spec['owned_hash_set_sha256']}`\n\nNo semantic implementation is permitted during materialization.\n")
    pre = {"schema":"vaeg-upd9002-m65-preflight-v1","campaign_base_sha":BASE,"tasks":specs,"execution_order":["M65j","M65a","M65b","M65c","M65d","M65e","M65f","M65g","M65h","M65i","M65k","M65l","M65m"],"semantic_start_permitted":False,"blockers":["M65a–M65e lack complete expected/actual case tables in approved G65 evidence","M65h remains evidence-blocked"]}
    (ROOT / "tests/ssts/campaigns/g65m/execution_spec_preflight.json").write_text(json.dumps(pre, sort_keys=True, indent=2) + "\n")
    print("materialized", len(specs), "tasks; semantic_start_permitted=false")

if __name__ == "__main__":
    main()
