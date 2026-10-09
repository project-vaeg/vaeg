# M65m — fingerprint-only residue and G65m campaign closure

M65 residue campaign is complete on one linear history.

Intermediate campaign checkpoints were technically validated but not independently approved.

The complete campaign is presented for one terminal human approval at G65m.

G65m remains unapproved pending human review.

M66 and M67 have not been started.

## Campaign identity

- Approved base: G65 at `efd96b7e46717e7ee56e086f7d27ba42b04b49d3`
- Branch: `topic/m65-residue-campaign`
- Campaign protocol SHA: `302540c5dff776890f205059235d3710d53a1636`
- M65m evaluated SHA: `a617889a0351918d081839beeb7f71a251e50f57`
- Hosted CI validator SHA: `28e35bd096e6d17256d9bef9693628daffa0215c`
- Evidence/final candidate SHA: supplied by final handoff after this evidence commit
- Worker SHA-256: `26892c7321f68804c19989867323eb7496a9f21a30f45af4e2df860e37c53340`
- Target policy: `upd9002-g64-37ae2b706a9cbbe2d36cf7c98372c0cae7ca4b8d90e4f738973bc0ed3248eed6`

## Completed checkpoints

| Node | Checkpoint SHA | Result |
| :--- | :--- | :--- |
| M65j_amended | `78df6d5a56ce646f8cb4f3aefd32b32b067c4377` | complete_pending_campaign_gate |
| M65a | `057489a98aac5f976b82530916d15c73541036a5` | complete_pending_campaign_gate |
| M65b | `e5ff4fda663156836d327314df28dd48c2006668` | complete_pending_campaign_gate |
| M65c | `ef7d88938944532606c46bf1d6032ccdfd635c6a` | complete_pending_campaign_gate |
| M65d | `ef44acbf5183ac5a8233ac007b07de72fd61eae8` | complete_pending_campaign_gate |
| M65e | `8350ca5d8345f3414e1864dcb6d70e391ea60cc1` | complete_pending_campaign_gate |
| M65f | `63dc2dba2427268e3af288e001cbe2121dbbf408` | complete_pending_campaign_gate |
| M65g | `2f3a49aaaf0fa905afe4146d815f684b86494ecd` | complete_pending_campaign_gate |
| M65h | `6aa3d179d49337bcf7d58b190ccd6c280c1dbadc` | complete_pending_campaign_gate |
| M65i | `054d3de07331153729d2dcbf22e9016517171457` | complete_pending_campaign_gate |
| M65k | `84f7974095b3335879a01d23a800b0e25dc52447` | complete_pending_campaign_gate |
| M65l | `a617889a0351918d081839beeb7f71a251e50f57` | complete_pending_campaign_gate |
| M65m | supplied by final handoff | terminal closure complete_pending_campaign_gate |

## Final profile results

| Profile | Selected | Applicable/executed | Pass | Fail | Timeout | Crash |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: |
| Architectural CI | 180,000 | 169,300 | 169,300 | 0 | 0 | 0 |
| Architectural full | 1,562,502 | 1,474,594 | 1,474,594 | 0 | 0 | 0 |
| Fingerprint full | 1,562,502 | 1,474,594 | 1,402,202 | 72,392 | 0 | 0 |

The architectural full profile reaches zero applicable failures: `1,474,594 pass / 0 fail`.
The fingerprint full profile remains diagnostic-only with `72,392` failures.

## Architectural residue closure

- Original G65 architectural failures: 7,511.
- Final original-residue replay: 7,511 pass / 0 fail.
- Direct architectural full newly passing: 7,511.
- Newly failing: 0.
- Timeout/crash: 0 / 0.

Owned populations:

- M65a `FF /7`: 5,000, digest `6028d5dcd4b6a3dcded2aaf69fb186e502f7f5a4d094180572f802c86240039a`.
- M65b `62 BOUND`: 1,244, digest `2fd0e1053b264042031c657ebf55796858e8ff2405509b3cc1d17ace71ae4f0d`.
- M65c `F7 /2`: 1,113, digest `69bf316c8a0751f7aed67504d0ea606fd2530e8d254b2b4e73ead66ccbc30ccc`.
- M65d `FF /6`: 144, digest `ce1bc644ee5a5bc73ae872440ad4446cb0dbccbad626ba93372082fe7add9076`.
- M65e exact tail: 10, digest `7b228418bf0391884381514282e60ea9ccaf3af8c0f1f7f5a1b038a24de230a1`.

## Implementation-missing and evidence backlog

The amended M65j 5,908-hash set remains exact:

- Hash-set SHA-256: `240e0bf76de968b310ad13ef53de8d044637b185e267e1cfb2540f32ab6571e5`.
- Classification: `known_target_gap`.
- Gap kind: `target_support_unverified`.
- Disposition: `approved_nonblocking_defer`.
- Implemented/applicable/executed/passing claim: false/false/false/false.

Zero-coverage and evidence-backlog items are not claimed passing. BRKEM and BRKFEM remain deferred under their recorded evidence/corpus gates.

## Transition identities

- Architectural CI transition digest: `858910af073261ec4a554ea6af112609c4e4fc3ce31aab397056967c4ebf098b`
- Architectural full transition digest: `a1fc54f790ddfced12da3568af17362ef2380f6a2d212c485ddf773cec3c08ac`
- Direct/composed state digest: `e0209980079b515d53a80d73035d6827c8c7a24c8ab910f3e4fb3091064fbec6`
- Ownership digest: `a3414f9d332f10dc3b1d748e6627fa0b0b0896f4ca1efe395a39b522a32e9ca8`
- M65j evidence-backlog digest: `240e0bf76de968b310ad13ef53de8d044637b185e267e1cfb2540f32ab6571e5`
- Architectural ranking digest: `2202ebcf0a4e1d9d93e519c80d2494c4cd9501e53bbb7d4111a4b0ee811fd6cb`
- Fingerprint ranking digest: `004422b52738f01f164154c000758c98a3a8c84cbee74245dca3111445a5bf7e`

## Validation summary

- Architectural CI/full/fingerprint full profiles completed with zero timeout and zero crash.
- Target policy, classifications, selected sets, applicable sets, fixtures, and comparison contracts are unchanged.
- Direct G65-to-G65m transition equals the composed checkpoint transition state.
- Native and repository validators are recorded in the final handoff.
- Hosted CI is required for the final pushed candidate; the exact URL/result is supplied in the final handoff.
- Corrective hosted-CI validation: run `30324791046` at
  `2bf7db5e7c0d4626537ee5bd151585ba05cb3de6` failed only because the
  external architectural CI ratchet was still bound to committed G64 evidence.
  The validator-only SHA above retargets that hosted check to committed G65m
  campaign evidence. Local direct and CTest `vaeg_upd9002_ssts_ci_external`
  checks both produced `180,000 selected / 169,300 executed / 169,300 pass /
  0 fail / 0 timeout / 0 crash`.

Standalone M65a through M65d checkpoint validators intentionally encode the
predecessor guard state that existed when each checkpoint was created. At the
terminal G65m head, those guards are superseded by later owned fixes: M65d
resolves the FF `/6` population that M65a through M65c guarded as unchanged,
and M65e resolves the ten-case tail that M65d guarded as unchanged. Terminal
closure therefore uses the final M65e validator, cumulative protection
manifest, direct/composed transition equality, and full architectural
zero-failure profiles instead of treating those predecessor-only guard
failures as regressions.

## Human-review commands

```bash
git fetch origin
git checkout topic/m65-residue-campaign
git status --short
python3 tools/qa/upd9002_m65_reconstruct.py verify --root .
python3 tools/qa/milestone_ids.py --selftest --audit --discover
python3 -m json.tool tests/ssts/scoreboard/g65m_architectural_full.json >/tmp/g65m_architectural_full.jsoncheck
git diff --check
```

## Known limitations

- M65m does not approve G65m; it only prepares the terminal candidate.
- Fingerprint-only residue remains diagnostic and is not used as blocking architectural work.
- M66 may begin only after formal G65m approval at the final 40-hex candidate SHA.
