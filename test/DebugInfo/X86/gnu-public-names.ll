; RUN: llc -mtriple=x86_64-pc-linux-gnu -generate-gnu-dwarf-pub-sections < %s | FileCheck -check-prefix=ASM %s
; RUN: llc -mtriple=x86_64-pc-linux-gnu -generate-gnu-dwarf-pub-sections -filetype=obj < %s | llvm-dwarfdump - | FileCheck %s
; ModuleID = 'dwarf-public-names.cpp'
;
; Generated from:
;
; struct C {
;   void member_function();
;   static int static_member_function();
;   static int static_member_variable;
; };
;
; int C::static_member_variable = 0;
;
; void C::member_function() {
;   static_member_variable = 0;
; }
;
; int C::static_member_function() {
;   return static_member_variable;
; }
;
; C global_variable;
;
; int global_function() {
;   return -1;
; }
;
; namespace ns {
;   void global_namespace_function() {
;     global_variable.member_function();
;   }
;   int global_namespace_variable = 1;
;   struct D {
;     int A;
;   } d;
; }

; ASM: .section        .debug_gnu_pubnames
; ASM: .byte   32                      # Kind: VARIABLE, EXTERNAL
; ASM-NEXT: .asciz  "global_variable"       # External Name

; ASM: .section        .debug_gnu_pubtypes
; ASM: .byte   16                      # Kind: TYPE, EXTERNAL
; ASM-NEXT: .asciz  "C"                     # External Name

; CHECK: .debug_info contents:
; CHECK: Compile Unit:
; CHECK: DW_AT_GNU_pubnames [DW_FORM_flag_present]   (true)
; CHECK-NOT: DW_AT_GNU_pubtypes [

; CHECK: [[STATIC_MEM_VAR:0x[0-9a-f]+]]: DW_TAG_variable
; CHECK-NEXT: DW_AT_specification {{.*}} "static_member_variable"

; CHECK: [[C:0x[0-9a-f]+]]: DW_TAG_structure_type
; CHECK-NEXT: DW_AT_name {{.*}} "C"

; CHECK: DW_TAG_member
; CHECK-NEXT: DW_AT_name {{.*}} "static_member_variable"

; CHECK: DW_TAG_subprogram
; CHECK-NEXT: DW_AT_linkage_name
; CHECK-NEXT: DW_AT_name {{.*}} "member_function"

; CHECK: DW_TAG_subprogram
; CHECK-NEXT: DW_AT_linkage_name
; CHECK-NEXT: DW_AT_name {{.*}} "static_member_function"

; CHECK: [[INT:0x[0-9a-f]+]]: DW_TAG_base_type
; CHECK-NEXT: DW_AT_name {{.*}} "int"

; CHECK: [[GLOB_VAR:0x[0-9a-f]+]]: DW_TAG_variable
; CHECK-NEXT: DW_AT_name {{.*}} "global_variable"

; CHECK: [[NS:0x[0-9a-f]+]]: DW_TAG_namespace
; CHECK-NEXT: DW_AT_name {{.*}} "ns"

; CHECK: [[GLOB_NS_VAR:0x[0-9a-f]+]]: DW_TAG_variable
; CHECK-NEXT: DW_AT_name {{.*}} "global_namespace_variable"
; CHECK-NOT: DW_AT_specification
; CHECK: DW_AT_location
; CHECK-NOT: DW_AT_specification

; CHECK: [[D_VAR:0x[0-9a-f]+]]: DW_TAG_variable
; CHECK-NEXT: DW_AT_name {{.*}} "d"
; CHECK-NOT: DW_AT_specification
; CHECK: DW_AT_location
; CHECK-NOT: DW_AT_specification

; CHECK: [[D:0x[0-9a-f]+]]: DW_TAG_structure_type
; CHECK-NEXT: DW_AT_name {{.*}} "D"

; CHECK: [[GLOB_NS_FUNC:0x[0-9a-f]+]]: DW_TAG_subprogram
; CHECK-NOT: DW_TAG
; CHECK: DW_AT_linkage_name
; CHECK-NOT: DW_TAG
; CHECK: DW_AT_name {{.*}} "global_namespace_function"

; CHECK: DW_TAG_subprogram
; CHECK-NOT: DW_TAG
; CHECK:   DW_AT_name {{.*}} "f3"
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK: [[F3_Z:.*]]:   DW_TAG_variable
; CHECK-NOT: DW_TAG
; CHECK:     DW_AT_name {{.*}} "z"
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK:     DW_AT_location
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK:   NULL
; CHECK-NOT: {{DW_TAG|NULL}}

; CHECK: [[ANON:.*]]: DW_TAG_namespace
; CHECK-NOT:   DW_AT_name
; CHECK: [[ANON_I:.*]]: DW_TAG_variable
; CHECK-NOT: DW_TAG
; CHECK:     DW_AT_name {{.*}} "i"
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK: [[ANON_INNER:.*]]:  DW_TAG_namespace
; CHECK-NOT: DW_TAG
; CHECK:     DW_AT_name {{.*}} "inner"
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK: [[ANON_INNER_B:.*]]: DW_TAG_variable
; CHECK-NOT: DW_TAG
; CHECK:       DW_AT_name {{.*}} "b"
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK:     NULL
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK:   NULL
; CHECK-NOT: {{DW_TAG|NULL}}

; CHECK: [[OUTER:.*]]: DW_TAG_namespace
; CHECK-NOT: DW_TAG
; CHECK:   DW_AT_name {{.*}} "outer"
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK: [[OUTER_ANON:.*]]:  DW_TAG_namespace
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK-NOT:     DW_AT_name
; CHECK: [[OUTER_ANON_C:.*]]: DW_TAG_variable
; CHECK-NOT: DW_TAG
; CHECK:       DW_AT_name {{.*}} "c"
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK:     NULL
; CHECK-NOT: {{DW_TAG|NULL}}
; FIXME: We probably shouldn't bother describing the implicit
; import of the preceding anonymous namespace. This should be fixed
; in clang.
; CHECK:     DW_TAG_imported_module
; CHECK-NOT: {{DW_TAG|NULL}}
; CHECK:   NULL
; CHECK-NOT: {{DW_TAG|NULL}}

; CHECK: [[MEM_FUNC:0x[0-9a-f]+]]: DW_TAG_subprogram
; CHECK-NOT: DW_TAG
; CHECK: DW_AT_specification {{.*}} "_ZN1C15member_functionEv"

; CHECK: [[STATIC_MEM_FUNC:0x[0-9a-f]+]]: DW_TAG_subprogram
; CHECK-NOT: DW_TAG
; CHECK: DW_AT_specification {{.*}} "_ZN1C22static_member_functionEv"

; CHECK: [[GLOBAL_FUNC:0x[0-9a-f]+]]: DW_TAG_subprogram
; CHECK-NOT: DW_TAG
; CHECK: DW_AT_linkage_name
; CHECK-NOT: DW_TAG
; CHECK: DW_AT_name {{.*}} "global_function"

; CHECK-LABEL: .debug_gnu_pubnames contents:
; CHECK-NEXT: length = {{.*}} version = 0x0002 unit_offset = 0x00000000 unit_size = {{.*}}
; CHECK-NEXT: Offset     Linkage  Kind     Name
; CHECK-DAG:  [[GLOBAL_FUNC]] EXTERNAL FUNCTION "global_function"
; CHECK-DAG:  [[NS]] EXTERNAL TYPE     "ns"
; CHECK-DAG:  [[MEM_FUNC]] EXTERNAL FUNCTION "C::member_function"
; CHECK-DAG:  [[GLOB_VAR]] EXTERNAL VARIABLE "global_variable"
; CHECK-DAG:  [[GLOB_NS_VAR]] EXTERNAL VARIABLE "ns::global_namespace_variable"
; CHECK-DAG:  [[GLOB_NS_FUNC]] EXTERNAL FUNCTION "ns::global_namespace_function"
; CHECK-DAG:  [[D_VAR]] EXTERNAL VARIABLE "ns::d"
; CHECK-DAG:  [[STATIC_MEM_VAR]] EXTERNAL VARIABLE "C::static_member_variable"
; CHECK-DAG:  [[STATIC_MEM_FUNC]] EXTERNAL FUNCTION "C::static_member_function"
; CHECK-DAG:  [[ANON]] EXTERNAL TYPE "(anonymous namespace)"
; CHECK-DAG:  [[ANON_INNER]] EXTERNAL TYPE "(anonymous namespace)::inner"
; CHECK-DAG:  [[OUTER]] EXTERNAL TYPE "outer"
; CHECK-DAG:  [[OUTER_ANON]] EXTERNAL TYPE "outer::(anonymous namespace)"
; CHECK-DAG:  [[ANON_I]] STATIC VARIABLE "(anonymous namespace)::i"
; CHECK-DAG:  [[ANON_INNER_B]] STATIC VARIABLE "(anonymous namespace)::inner::b"
; CHECK-DAG:  [[OUTER_ANON_C]] STATIC VARIABLE "outer::(anonymous namespace)::c"

; GCC Doesn't put local statics in pubnames, but it seems not unreasonable and
; comes out naturally from LLVM's implementation, so I'm OK with it for now. If
; it's demonstrated that this is a major size concern or degrades debug info
; consumer behavior, feel free to change it.

; CHECK-DAG:  [[F3_Z]] STATIC VARIABLE "f3::z"


; CHECK-LABEL: debug_gnu_pubtypes contents:
; CHECK: Offset     Linkage  Kind     Name
; CHECK-DAG:  [[C]] EXTERNAL TYPE     "C"
; CHECK-DAG:  [[D]] EXTERNAL TYPE     "ns::D"
; CHECK-DAG:  [[INT]] STATIC   TYPE     "int"

%struct.C = type { i8 }
%"struct.ns::D" = type { i32 }

@_ZN1C22static_member_variableE = global i32 0, align 4
@global_variable = global %struct.C zeroinitializer, align 1
@_ZN2ns25global_namespace_variableE = global i32 1, align 4
@_ZN2ns1dE = global %"struct.ns::D" zeroinitializer, align 4
@_ZZ2f3vE1z = internal global i32 0, align 4
@_ZN12_GLOBAL__N_11iE = internal global i32 0, align 4
@_ZN12_GLOBAL__N_15inner1bE = internal global i32 0, align 4
@_ZN5outer12_GLOBAL__N_11cE = internal global i32 0, align 4

; Function Attrs: nounwind uwtable
define void @_ZN1C15member_functionEv(%struct.C* %this) #0 align 2 {
entry:
  %this.addr = alloca %struct.C*, align 8
  store %struct.C* %this, %struct.C** %this.addr, align 8
  call void @llvm.dbg.declare(metadata %struct.C** %this.addr, metadata !49, metadata !51), !dbg !52
  %this1 = load %struct.C*, %struct.C** %this.addr
  store i32 0, i32* @_ZN1C22static_member_variableE, align 4, !dbg !53
  ret void, !dbg !54
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: nounwind uwtable
define i32 @_ZN1C22static_member_functionEv() #0 align 2 {
entry:
  %0 = load i32, i32* @_ZN1C22static_member_variableE, align 4, !dbg !55
  ret i32 %0, !dbg !55
}

; Function Attrs: nounwind uwtable
define i32 @_Z15global_functionv() #0 {
entry:
  ret i32 -1, !dbg !56
}

; Function Attrs: nounwind uwtable
define void @_ZN2ns25global_namespace_functionEv() #0 {
entry:
  call void @_ZN1C15member_functionEv(%struct.C* @global_variable), !dbg !57
  ret void, !dbg !58
}

; Function Attrs: nounwind uwtable
define i32* @_Z2f3v() #0 {
entry:
  ret i32* @_ZZ2f3vE1z, !dbg !59
}

; Function Attrs: nounwind uwtable
define i32 @_Z2f7v() #0 {
entry:
  %0 = load i32, i32* @_ZN12_GLOBAL__N_11iE, align 4, !dbg !60
  %call = call i32* @_Z2f3v(), !dbg !60
  %1 = load i32, i32* %call, align 4, !dbg !60
  %add = add nsw i32 %0, %1, !dbg !60
  %2 = load i32, i32* @_ZN12_GLOBAL__N_15inner1bE, align 4, !dbg !60
  %add1 = add nsw i32 %add, %2, !dbg !60
  %3 = load i32, i32* @_ZN5outer12_GLOBAL__N_11cE, align 4, !dbg !60
  %add2 = add nsw i32 %add1, %3, !dbg !60
  ret i32 %add2, !dbg !60
}

attributes #0 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!46, !47}
!llvm.ident = !{!48}

!0 = !MDCompileUnit(language: DW_LANG_C_plus_plus, file: !1, producer: "clang version 3.7.0 (trunk 234754) (llvm/trunk 234760)", isOptimized: false, runtimeVersion: 0, emissionKind: 1, enums: !2, retainedTypes: !3, subprograms: !19, globals: !31, imports: !44)
!1 = !MDFile(filename: "gnu-public-names.cpp", directory: "/tmp/dbginfo")
!2 = !{}
!3 = !{!4, !15}
!4 = !MDCompositeType(tag: DW_TAG_structure_type, name: "C", file: !1, line: 1, size: 8, align: 8, elements: !5, identifier: "_ZTS1C")
!5 = !{!6, !8, !12}
!6 = !MDDerivedType(tag: DW_TAG_member, name: "static_member_variable", scope: !"_ZTS1C", file: !1, line: 4, baseType: !7, flags: DIFlagStaticMember)
!7 = !MDBasicType(name: "int", size: 32, align: 32, encoding: DW_ATE_signed)
!8 = !MDSubprogram(name: "member_function", linkageName: "_ZN1C15member_functionEv", scope: !"_ZTS1C", file: !1, line: 2, type: !9, isLocal: false, isDefinition: false, scopeLine: 2, flags: DIFlagPrototyped, isOptimized: false)
!9 = !MDSubroutineType(types: !10)
!10 = !{null, !11}
!11 = !MDDerivedType(tag: DW_TAG_pointer_type, baseType: !"_ZTS1C", size: 64, align: 64, flags: DIFlagArtificial | DIFlagObjectPointer)
!12 = !MDSubprogram(name: "static_member_function", linkageName: "_ZN1C22static_member_functionEv", scope: !"_ZTS1C", file: !1, line: 3, type: !13, isLocal: false, isDefinition: false, scopeLine: 3, flags: DIFlagPrototyped, isOptimized: false)
!13 = !MDSubroutineType(types: !14)
!14 = !{!7}
!15 = !MDCompositeType(tag: DW_TAG_structure_type, name: "D", scope: !16, file: !1, line: 28, size: 32, align: 32, elements: !17, identifier: "_ZTSN2ns1DE")
!16 = !MDNamespace(name: "ns", scope: null, file: !1, line: 23)
!17 = !{!18}
!18 = !MDDerivedType(tag: DW_TAG_member, name: "A", scope: !"_ZTSN2ns1DE", file: !1, line: 29, baseType: !7, size: 32, align: 32)
!19 = !{!20, !21, !22, !23, !26, !30}
!20 = !MDSubprogram(name: "member_function", linkageName: "_ZN1C15member_functionEv", scope: !"_ZTS1C", file: !1, line: 9, type: !9, isLocal: false, isDefinition: true, scopeLine: 9, flags: DIFlagPrototyped, isOptimized: false, function: void (%struct.C*)* @_ZN1C15member_functionEv, declaration: !8, variables: !2)
!21 = !MDSubprogram(name: "static_member_function", linkageName: "_ZN1C22static_member_functionEv", scope: !"_ZTS1C", file: !1, line: 13, type: !13, isLocal: false, isDefinition: true, scopeLine: 13, flags: DIFlagPrototyped, isOptimized: false, function: i32 ()* @_ZN1C22static_member_functionEv, declaration: !12, variables: !2)
!22 = !MDSubprogram(name: "global_function", linkageName: "_Z15global_functionv", scope: !1, file: !1, line: 19, type: !13, isLocal: false, isDefinition: true, scopeLine: 19, flags: DIFlagPrototyped, isOptimized: false, function: i32 ()* @_Z15global_functionv, variables: !2)
!23 = !MDSubprogram(name: "global_namespace_function", linkageName: "_ZN2ns25global_namespace_functionEv", scope: !16, file: !1, line: 24, type: !24, isLocal: false, isDefinition: true, scopeLine: 24, flags: DIFlagPrototyped, isOptimized: false, function: void ()* @_ZN2ns25global_namespace_functionEv, variables: !2)
!24 = !MDSubroutineType(types: !25)
!25 = !{null}
!26 = !MDSubprogram(name: "f3", linkageName: "_Z2f3v", scope: !1, file: !1, line: 37, type: !27, isLocal: false, isDefinition: true, scopeLine: 37, flags: DIFlagPrototyped, isOptimized: false, function: i32* ()* @_Z2f3v, variables: !2)
!27 = !MDSubroutineType(types: !28)
!28 = !{!29}
!29 = !MDDerivedType(tag: DW_TAG_pointer_type, baseType: !7, size: 64, align: 64)
!30 = !MDSubprogram(name: "f7", linkageName: "_Z2f7v", scope: !1, file: !1, line: 54, type: !13, isLocal: false, isDefinition: true, scopeLine: 54, flags: DIFlagPrototyped, isOptimized: false, function: i32 ()* @_Z2f7v, variables: !2)
!31 = !{!32, !33, !34, !35, !36, !37, !39, !41}
!32 = !MDGlobalVariable(name: "static_member_variable", linkageName: "_ZN1C22static_member_variableE", scope: !0, file: !1, line: 7, type: !7, isLocal: false, isDefinition: true, variable: i32* @_ZN1C22static_member_variableE, declaration: !6)
!33 = !MDGlobalVariable(name: "global_variable", scope: !0, file: !1, line: 17, type: !"_ZTS1C", isLocal: false, isDefinition: true, variable: %struct.C* @global_variable)
!34 = !MDGlobalVariable(name: "global_namespace_variable", linkageName: "_ZN2ns25global_namespace_variableE", scope: !16, file: !1, line: 27, type: !7, isLocal: false, isDefinition: true, variable: i32* @_ZN2ns25global_namespace_variableE)
!35 = !MDGlobalVariable(name: "d", linkageName: "_ZN2ns1dE", scope: !16, file: !1, line: 30, type: !"_ZTSN2ns1DE", isLocal: false, isDefinition: true, variable: %"struct.ns::D"* @_ZN2ns1dE)
!36 = !MDGlobalVariable(name: "z", scope: !26, file: !1, line: 38, type: !7, isLocal: true, isDefinition: true, variable: i32* @_ZZ2f3vE1z)
!37 = !MDGlobalVariable(name: "i", linkageName: "_ZN12_GLOBAL__N_11iE", scope: !38, file: !1, line: 34, type: !7, isLocal: true, isDefinition: true, variable: i32* @_ZN12_GLOBAL__N_11iE)
!38 = !MDNamespace(scope: null, file: !1, line: 33)
!39 = !MDGlobalVariable(name: "b", linkageName: "_ZN12_GLOBAL__N_15inner1bE", scope: !40, file: !1, line: 44, type: !7, isLocal: true, isDefinition: true, variable: i32* @_ZN12_GLOBAL__N_15inner1bE)
!40 = !MDNamespace(name: "inner", scope: !38, file: !1, line: 43)
!41 = !MDGlobalVariable(name: "c", linkageName: "_ZN5outer12_GLOBAL__N_11cE", scope: !42, file: !1, line: 50, type: !7, isLocal: true, isDefinition: true, variable: i32* @_ZN5outer12_GLOBAL__N_11cE)
!42 = !MDNamespace(scope: !43, file: !1, line: 49)
!43 = !MDNamespace(name: "outer", scope: null, file: !1, line: 48)
!44 = !{!45}
!45 = !MDImportedEntity(tag: DW_TAG_imported_module, scope: !43, entity: !42, line: 40)
!46 = !{i32 2, !"Dwarf Version", i32 4}
!47 = !{i32 2, !"Debug Info Version", i32 3}
!48 = !{!"clang version 3.7.0 (trunk 234754) (llvm/trunk 234760)"}
!49 = !MDLocalVariable(tag: DW_TAG_arg_variable, name: "this", arg: 1, scope: !20, type: !50, flags: DIFlagArtificial | DIFlagObjectPointer)
!50 = !MDDerivedType(tag: DW_TAG_pointer_type, baseType: !"_ZTS1C", size: 64, align: 64)
!51 = !MDExpression()
!52 = !MDLocation(line: 0, scope: !20)
!53 = !MDLocation(line: 10, scope: !20)
!54 = !MDLocation(line: 11, scope: !20)
!55 = !MDLocation(line: 14, scope: !21)
!56 = !MDLocation(line: 20, scope: !22)
!57 = !MDLocation(line: 25, scope: !23)
!58 = !MDLocation(line: 26, scope: !23)
!59 = !MDLocation(line: 39, scope: !26)
!60 = !MDLocation(line: 55, scope: !30)
