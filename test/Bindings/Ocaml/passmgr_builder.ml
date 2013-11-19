(* RUN: rm -rf %t.builddir
 * RUN: mkdir -p %t.builddir
 * RUN: cp %s %t.builddir
 * RUN: %ocamlopt -warn-error A llvm.cmxa llvm_passmgr_builder.cmxa %t.builddir/passmgr_builder.ml -o %t
 * RUN: %t %t.bc
 * XFAIL: vg_leak
 *)

(* Note: It takes several seconds for ocamlopt to link an executable with
         libLLVMCore.a, so it's better to write a big test than a bunch of
         little ones. *)

open Llvm
open Llvm_passmgr_builder

let context = global_context ()
let void_type = Llvm.void_type context

(* Tiny unit test framework - really just to help find which line is busted *)
let print_checkpoints = false

let suite name f =
  if print_checkpoints then
    prerr_endline (name ^ ":");
  f ()


(*===-- Fixture -----------------------------------------------------------===*)

let filename = Sys.argv.(1)
let m = create_module context filename


(*===-- Pass Manager Builder ----------------------------------------------===*)

let test_pmbuilder () =
  let (++) x f = ignore (f x); x in

  let module_passmgr = PassManager.create () in
  let func_passmgr   = PassManager.create_function m in
  let lto_passmgr    = PassManager.create () in

  ignore (Llvm_passmgr_builder.create ()
           ++ set_opt_level 3
           ++ set_size_level 1
           ++ set_disable_unit_at_a_time false
           ++ set_disable_unroll_loops false
           ++ use_inliner_with_threshold 10
           ++ populate_function_pass_manager func_passmgr
           ++ populate_module_pass_manager module_passmgr
           ++ populate_lto_pass_manager lto_passmgr
                  ~internalize:false ~run_inliner:false);
  Gc.compact ();

  PassManager.dispose module_passmgr;
  PassManager.dispose func_passmgr;
  PassManager.dispose lto_passmgr


(*===-- Driver ------------------------------------------------------------===*)

let _ =
  suite "pass manager builder" test_pmbuilder;
  dispose_module m
