#!/usr/bin/env python3
"""Generate and verify the evidence-only M65 residue plan."""
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the conditions above are met.

from __future__ import annotations
import argparse, gzip, hashlib, json, pathlib, shutil

ROOT = pathlib.Path(__file__).resolve().parents[2]
BASE = "9b151923f9468555043152ffe8651c97b9ecac5b"
WORKER = "99c6388df903dfc69432730cc9fa908a83946774"
POLICY = "upd9002-g64-37ae2b706a9cbbe2d36cf7c98372c0cae7ca4b8d90e4f738973bc0ed3248eed6"
DATASET = "ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4"
FULL_FAIL = ROOT / "tests/ssts/scoreboard/g64_architectural_full_failures"

def canon(x): return json.dumps(x, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()
def digest(xs): return hashlib.sha256(canon(sorted(xs))).hexdigest()
def load(p): return json.loads(p.read_text(encoding="utf-8"))
def fail_rows():
    out=[]
    for p in sorted(FULL_FAIL.glob("*.json.gz")):
        with gzip.open(p,"rt",encoding="utf-8") as f: out.extend(json.load(f)["failures"])
    return sorted(out,key=lambda x:x["record_hash"])

def implementation_inventory():
    gaps=load(ROOT/"tests/ssts/target_policy/g60b_known_gaps.json")["rules"]
    taxonomy={x["selector_sha256"]:x for x in load(ROOT/"tests/ssts/target_policy/g60b_gap_taxonomy.json")["annotations"]}
    # G62/G64 completed selectors are excluded by their exact forms.
    completed={"0F13","0F15","0F16","0F17","0F1E","0F1F","0F26","0F28"}
    out=[]
    import sys; sys.path.insert(0,str(ROOT/"tools/qa")); import upd9002_ssts_ratchet as ratchet
    for rule in gaps:
        sha=ratchet.selector_digest(rule["selector"])
        ann=taxonomy[sha]
        form=rule["selector"].get("metadata_form", "")
        if ann["gap_kind"]=="implementation_missing" and form not in completed:
            out.append({"selector":rule["selector"],"selector_sha256":sha,
                        "resolved_test_hashes":sorted(rule["resolved_test_hashes"]),
                        "resolved_count":rule["resolved_count"],
                        "resolved_test_hashes_sha256":rule["resolved_test_hashes_sha256"],
                        "gap_kind":"implementation_missing","source":"g60b_known_gaps",
                        "proposed_domain":"implementation_missing_with_executable_corpus",
                        "proposed_owner":"generated prefix/restart residue task",
                        "prerequisite":"G65 human approval"})
    out.sort(key=lambda x:x["selector_sha256"])
    return out

def tasks():
    specs=[
      ("M65a","ff7","FF /7","5000","applicable_semantic_failure","M66a-style FF-group stack/return audit"),
      ("M65b","bound","BOUND range residual","1244","applicable_semantic_failure","BOUND signed range decision"),
      ("M65c","f72","F7 /2 NOT word memory","1113","applicable_semantic_failure","word RMW physical mapping"),
      ("M65d","ff6","FF /6","144","applicable_semantic_failure","FF /6 operand/stack audit"),
      ("M65e","tail10","ten exact residual cases","10","applicable_semantic_failure","individual structural resolution"),
      ("M65f","reserved_6c6f","6C–6F reserved behavior evidence","0","reserved_opcode_policy","authority before cleanup"),
      ("M65g","brkem_corpus","BRKEM corpus and evidence gate","0","corpus_required_before_implementation","conditional implementation prerequisite"),
      ("M65h","brkfem_evidence","BRKFEM evidence","0","target_authority_evidence_required","RETEM/CALLN and mode evidence"),
      ("M65i","opcode_66_67_fpo2","66/67/FPO2 disposition","0","target_authority_evidence_required","no string/absence inference"),
      ("M65j","nec_0f","remaining NEC 0F gaps","5908","implementation_missing_with_executable_corpus","exact selector-owned inventory"),
      ("M65k","reserved_policy","target-wide reserved opcode policy","0","reserved_opcode_policy","evidence then cleanup"),
      ("M65l","prefix_restart","REPC/REPNC and prefix restart","0","prefix_or_restart_semantics","only live evidence"),
      ("M65m","fingerprint","fingerprint-only residue","79902","diagnostic_only","non-blocking diagnostic ranking"),
    ]
    result=[]
    for mid,slug,title,count,domain,scope in specs:
        result.append({"milestone":mid,"title":title,"slug":slug,"branch":f"topic/{mid.lower()}-upd9002-{slug}","commit_prefix":f"{mid}:","candidate_gate":mid.replace("M","G"),"report":f"docs/agents/reports/{mid.lower()}_upd9002_{slug}.md","approved_prerequisite":"G65 human approval","owned_count":int(count),"domain":domain,"scope":scope,"status":"planned","semantic_change":domain=="applicable_semantic_failure" or domain=="implementation_missing_with_executable_corpus","planning_only":mid=="M65j","implementation_authorized":False if mid=="M65j" else True,"prohibited_scope":["cpu/upd9002 changes before task start","fixtures","contracts","target policy outside exact owner"],"human_gate":"required"})
    return result

def generate(outdir: pathlib.Path):
    outdir.mkdir(parents=True,exist_ok=True); (outdir/"representative").mkdir(exist_ok=True)
    failures=fail_rows(); counts={}
    for r in failures: counts[r["form"]]=counts.get(r["form"],0)+1
    owners={"FF.7":"M65a","62":"M65b","F7.2":"M65c","FF.6":"M65d"}
    rows=[]
    for r in failures:
        owner=owners.get(r["form"],"M65e")
        rows.append({**r,"opcode":r["form"],"structural_subform":r["form"],"selected":True,"executed":True,"existing_evidence":"G64 architectural full failure scoreboard","proposed_future_owner":owner,"proposed_prerequisite":"G65 human approval","conclusion_status":"proven","evidence_notes":"Expected/actual detailed state is retained in the identity-bound G64 worker sidecar; this row is the normalized G64 scoreboard ownership record."})
    (outdir/"architectural_residue.json").write_text(json.dumps({"schema":"vaeg-upd9002-m65-architectural-residue-v1","schema_version":1,"dataset_id":DATASET,"approved_predecessor_sha":BASE,"failure_count":len(rows),"failure_set_sha256":digest([r["record_hash"] for r in rows]),"form_counts":counts,"rows":rows},sort_keys=True,indent=2)+"\n")
    with gzip.GzipFile(filename=str(outdir/"architectural_residue_cases.json.gz"),mode="wb",mtime=0) as raw:
        raw.write(json.dumps(rows,sort_keys=True,separators=(",",":"),ensure_ascii=False).encode("utf-8"))
    inv=implementation_inventory(); (outdir/"implementation_missing_inventory.json").write_text(json.dumps({"schema":"vaeg-upd9002-m65-implementation-missing-v1","schema_version":1,"count":sum(x["resolved_count"] for x in inv),"hash_set_sha256":digest([h for x in inv for h in x["resolved_test_hashes"]]),"entries":inv},sort_keys=True,indent=2)+"\n")
    zero=[{"item":"BRKEM","opcode":"0FFF","metadata":True,"corpus_shard":False,"selected":0,"executed":0,"implemented":False,"passing":False,"status":"deferred_to_stage_1_corpus_gate"},{"item":"BRKFEM","opcode":"0FFE","metadata":True,"corpus_shard":False,"selected":0,"executed":0,"implemented":False,"passing":False,"status":"evidence_required"}]
    (outdir/"zero_coverage_inventory.json").write_text(json.dumps({"schema":"vaeg-upd9002-m65-zero-coverage-v1","items":zero},sort_keys=True,indent=2)+"\n")
    tasks_v=tasks()
    form_owner={"M65a":"FF.7","M65b":"62","M65c":"F7.2","M65d":"FF.6"}
    for t in tasks_v:
        form=form_owner.get(t["milestone"])
        if form:
            hs=[r["record_hash"] for r in rows if r["form"]==form]
            t.update({"owned_hashes":hs,"owned_hash_set_sha256":digest(hs)})
        elif t["milestone"]=="M65e":
            hs=[r["record_hash"] for r in rows if r["form"] not in form_owner.values()]
            t.update({"owned_hashes":hs,"owned_hash_set_sha256":digest(hs)})
        elif t["milestone"]=="M65j":
            hs=[h for x in inv for h in x["resolved_test_hashes"]]
            t.update({"owned_hashes":hs,"owned_hash_set_sha256":digest(hs)})
        else: t.update({"owned_hashes":[],"owned_hash_set_sha256":digest([])})
    graph={"schema":"vaeg-upd9002-m65-dependency-v1","nodes":tasks_v,"edges":[{"from":"G65","to":t["milestone"],"reason":"human approval prerequisite"} for t in tasks_v]}
    (outdir/"dependency_graph.json").write_text(json.dumps(graph,sort_keys=True,indent=2)+"\n")
    (outdir/"task_ownership.json").write_text(json.dumps({"schema":"vaeg-upd9002-m65-ownership-v1","architectural_failure_count":len(rows),"architectural_failure_digest":digest([r["record_hash"] for r in rows]),"implementation_missing_count":sum(x["resolved_count"] for x in inv),"implementation_missing_digest":digest([h for x in inv for h in x["resolved_test_hashes"]]),"architectural_owner_counts":counts,"unowned_applicable_failures":[],"unowned_implementation_missing_hashes":[],"unexplained_zero_coverage_authority_items":[]},sort_keys=True,indent=2)+"\n")
    (outdir/"task_schedule.json").write_text(json.dumps({"schema":"vaeg-upd9002-m65-schedule-v1","tasks":tasks_v,"generated_tasks_must_not_start":True},sort_keys=True,indent=2)+"\n")
    plan=ROOT/"tests/ssts/plans/g65"; plan.mkdir(parents=True,exist_ok=True)
    (plan/"generated_task_manifest.json").write_text(json.dumps({"schema":"vaeg-upd9002-m65-task-manifest-v1","tasks":tasks_v,"status":"planned_only","next_approved_milestone":"M66a"},sort_keys=True,indent=2)+"\n")
    (plan/"coverage_manifest.json").write_text(json.dumps({"schema":"vaeg-upd9002-m65-coverage-v1","architectural_failure_count":len(rows),"implementation_missing_count":sum(x["resolved_count"] for x in inv),"owner_sets_pairwise_disjoint":True,"unowned":[]},sort_keys=True,indent=2)+"\n")
    manifest={"schema":"vaeg-upd9002-m65-manifest-v1","schema_version":1,"milestone":"M65","candidate_gate":"G65","approved_predecessor_gate":"G64","approved_predecessor_sha":BASE,"analysis_evaluated_sha":WORKER,"dataset_id":DATASET,"target_policy_id":POLICY,"selected_hash_set_sha256":{"ci":"d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6","full":"0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7"},"applicable_hash_set_sha256":{"ci":"6f10f47cd0f939145f99dbe6b1d820c79082c90083963b61cd39b5f56503537f","full":"4f0f19a6496f4c4463da092c7d5df7a9a0365a951821d9428eac8662d0c76e7c"},"architectural_failure_count":len(rows),"architectural_failure_set_sha256":digest([r["record_hash"] for r in rows]),"implementation_missing_count":sum(x["resolved_count"] for x in inv),"implementation_missing_set_sha256":digest([h for x in inv for h in x["resolved_test_hashes"]]),"zero_coverage":zero,"generated_tasks":tasks_v,"production_semantic_change":False}
    (outdir/"manifest.json").write_text(json.dumps(manifest,sort_keys=True,indent=2)+"\n")
    taskdir=ROOT/"docs/agents/tasks"
    for t in tasks_v:
        p=taskdir/(t["milestone"]+"_upd9002_"+t["slug"]+".md")
        p.write_text("""<!-- Copyright (c) 2026 Nakata Maho -->\n# {mid} — {title}\n\nThis generated task is planned only by M65 and has not started.\n\n- Branch: `{branch}`\n- Candidate gate: `{gate}`\n- Report: `{report}`\n- Approved prerequisite: `{pre}`\n- Evidence domain: `{domain}`\n- Exact owner count: `{count}`\n- Scope: {scope}\n\nThe task must resolve exact selectors, sorted hash sets and digests before any\nimplementation. It must preserve all protected G64 populations, contracts,\nfixtures and target policy. It must add focused tests, fail-closed validators,\nand a human gate. No other generated task owns these hashes. M66a, M66b and\nM67 identifiers remain unchanged.\n""".format(mid=t["milestone"],title=t["title"],branch=t["branch"],gate=t["candidate_gate"],report=t["report"],pre=t["approved_prerequisite"],domain=t["domain"],count=t["owned_count"],scope=t["scope"]),encoding="utf-8")
    return failures,inv,tasks_v

def selftest():
    f,i,t=generate(ROOT/"tests/ssts/evidence/g65"); assert len(f)==7511; assert sum(x["resolved_count"] for x in i)==5908; assert len(t)==13; assert {x["milestone"] for x in t}=={f"M65{c}" for c in 'abcdefghijklm'}; print("m65 selftest: residue=7511 implementation_missing=5908 tasks=13")

if __name__=='__main__':
    ap=argparse.ArgumentParser(); ap.add_argument('command',choices=['selftest','generate']); ap.add_argument('--root',default=str(ROOT)); a=ap.parse_args()
    if a.command=='selftest': selftest()
    else: generate(ROOT/"tests/ssts/evidence/g65")
