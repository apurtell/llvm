; RUN: llc < %s -mtriple=x86_64-apple-darwin | not grep "movb   %ah, %r"

	%struct.FILE = type { i8*, i32, i32, i16, i16, %struct.__sbuf, i32, i8*, i32 (i8*)*, i32 (i8*, i8*, i32)*, i64 (i8*, i64, i32)*, i32 (i8*, i8*, i32)*, %struct.__sbuf, %struct.__sFILEX*, i32, [3 x i8], [1 x i8], %struct.__sbuf, i32, [4 x i8], i64 }
	%struct.PyBoolScalarObject = type { i64, %struct._typeobject*, i8 }
	%struct.PyBufferProcs = type { i64 (%struct.PyObject*, i64, i8**)*, i64 (%struct.PyObject*, i64, i8**)*, i64 (%struct.PyObject*, i64*)*, i64 (%struct.PyObject*, i64, i8**)* }
	%struct.PyGetSetDef = type { i8*, %struct.PyObject* (%struct.PyObject*, i8*)*, i32 (%struct.PyObject*, %struct.PyObject*, i8*)*, i8*, i8* }
	%struct.PyMappingMethods = type { i64 (%struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, i32 (%struct.PyObject*, %struct.PyObject*, %struct.PyObject*)* }
	%struct.PyMemberDef = type opaque
	%struct.PyMethodDef = type { i8*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, i32, i8* }
	%struct.PyNumberMethods = type { %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*)*, i32 (%struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, i32 (%struct.PyObject**, %struct.PyObject**)*, %struct.PyObject* (%struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*)* }
	%struct.PyObject = type { i64, %struct._typeobject* }
	%struct.PySequenceMethods = type { i64 (%struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, i64)*, %struct.PyObject* (%struct.PyObject*, i64)*, %struct.PyObject* (%struct.PyObject*, i64, i64)*, i32 (%struct.PyObject*, i64, %struct.PyObject*)*, i32 (%struct.PyObject*, i64, i64, %struct.PyObject*)*, i32 (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, i64)* }
	%struct.PyTupleObject = type { i64, %struct._typeobject*, i64, [1 x %struct.PyObject*] }
	%struct.__sFILEX = type opaque
	%struct.__sbuf = type { i8*, i32 }
	%struct._typeobject = type { i64, %struct._typeobject*, i64, i8*, i64, i64, void (%struct.PyObject*)*, i32 (%struct.PyObject*, %struct.FILE*, i32)*, %struct.PyObject* (%struct.PyObject*, i8*)*, i32 (%struct.PyObject*, i8*, %struct.PyObject*)*, i32 (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*)*, %struct.PyNumberMethods*, %struct.PySequenceMethods*, %struct.PyMappingMethods*, i64 (%struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, i32 (%struct.PyObject*, %struct.PyObject*, %struct.PyObject*)*, %struct.PyBufferProcs*, i64, i8*, i32 (%struct.PyObject*, i32 (%struct.PyObject*, i8*)*, i8*)*, i32 (%struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*, i32)*, i64, %struct.PyObject* (%struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*)*, %struct.PyMethodDef*, %struct.PyMemberDef*, %struct.PyGetSetDef*, %struct._typeobject*, %struct.PyObject*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*, %struct.PyObject*)*, i32 (%struct.PyObject*, %struct.PyObject*, %struct.PyObject*)*, i64, i32 (%struct.PyObject*, %struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct._typeobject*, i64)*, %struct.PyObject* (%struct._typeobject*, %struct.PyObject*, %struct.PyObject*)*, void (i8*)*, i32 (%struct.PyObject*)*, %struct.PyObject*, %struct.PyObject*, %struct.PyObject*, %struct.PyObject*, %struct.PyObject*, void (%struct.PyObject*)* }
@PyArray_API = external global i8**		; <i8***> [#uses=4]
@PyUFunc_API = external global i8**		; <i8***> [#uses=4]
@.str5 = external constant [14 x i8]		; <[14 x i8]*> [#uses=1]

define %struct.PyObject* @ubyte_divmod(%struct.PyObject* %a, %struct.PyObject* %b) {
entry:
	%arg1 = alloca i8, align 1		; <i8*> [#uses=3]
	%arg2 = alloca i8, align 1		; <i8*> [#uses=3]
	%first = alloca i32, align 4		; <i32*> [#uses=2]
	%bufsize = alloca i32, align 4		; <i32*> [#uses=1]
	%errmask = alloca i32, align 4		; <i32*> [#uses=2]
	%errobj = alloca %struct.PyObject*, align 8		; <%struct.PyObject**> [#uses=2]
	%tmp3.i = call fastcc i32 @_ubyte_convert_to_ctype( %struct.PyObject* %a, i8* %arg1 )		; <i32> [#uses=2]
	%tmp5.i = icmp slt i32 %tmp3.i, 0		; <i1> [#uses=1]
	br i1 %tmp5.i, label %_ubyte_convert2_to_ctypes.exit, label %cond_next.i

cond_next.i:		; preds = %entry
	%tmp11.i = call fastcc i32 @_ubyte_convert_to_ctype( %struct.PyObject* %b, i8* %arg2 )		; <i32> [#uses=2]
	%tmp13.i = icmp slt i32 %tmp11.i, 0		; <i1> [#uses=1]
	%retval.i = select i1 %tmp13.i, i32 %tmp11.i, i32 0		; <i32> [#uses=1]
	switch i32 %retval.i, label %bb35 [
		 i32 -2, label %bb17
		 i32 -1, label %bb4
	]

_ubyte_convert2_to_ctypes.exit:		; preds = %entry
	switch i32 %tmp3.i, label %bb35 [
		 i32 -2, label %bb17
		 i32 -1, label %bb4
	]

bb4:		; preds = %_ubyte_convert2_to_ctypes.exit, %cond_next.i
	%tmp5 = load i8**, i8*** @PyArray_API, align 8		; <i8**> [#uses=1]
	%tmp6 = getelementptr i8*, i8** %tmp5, i64 2		; <i8**> [#uses=1]
	%tmp7 = load i8*, i8** %tmp6		; <i8*> [#uses=1]
	%tmp78 = bitcast i8* %tmp7 to %struct._typeobject*		; <%struct._typeobject*> [#uses=1]
	%tmp9 = getelementptr %struct._typeobject, %struct._typeobject* %tmp78, i32 0, i32 12		; <%struct.PyNumberMethods**> [#uses=1]
	%tmp10 = load %struct.PyNumberMethods*, %struct.PyNumberMethods** %tmp9		; <%struct.PyNumberMethods*> [#uses=1]
	%tmp11 = getelementptr %struct.PyNumberMethods, %struct.PyNumberMethods* %tmp10, i32 0, i32 5		; <%struct.PyObject* (%struct.PyObject*, %struct.PyObject*)**> [#uses=1]
	%tmp12 = load %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)** %tmp11		; <%struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*> [#uses=1]
	%tmp15 = call %struct.PyObject* %tmp12( %struct.PyObject* %a, %struct.PyObject* %b )		; <%struct.PyObject*> [#uses=1]
	ret %struct.PyObject* %tmp15

bb17:		; preds = %_ubyte_convert2_to_ctypes.exit, %cond_next.i
	%tmp18 = call %struct.PyObject* @PyErr_Occurred( )		; <%struct.PyObject*> [#uses=1]
	%tmp19 = icmp eq %struct.PyObject* %tmp18, null		; <i1> [#uses=1]
	br i1 %tmp19, label %cond_next, label %UnifiedReturnBlock

cond_next:		; preds = %bb17
	%tmp22 = load i8**, i8*** @PyArray_API, align 8		; <i8**> [#uses=1]
	%tmp23 = getelementptr i8*, i8** %tmp22, i64 10		; <i8**> [#uses=1]
	%tmp24 = load i8*, i8** %tmp23		; <i8*> [#uses=1]
	%tmp2425 = bitcast i8* %tmp24 to %struct._typeobject*		; <%struct._typeobject*> [#uses=1]
	%tmp26 = getelementptr %struct._typeobject, %struct._typeobject* %tmp2425, i32 0, i32 12		; <%struct.PyNumberMethods**> [#uses=1]
	%tmp27 = load %struct.PyNumberMethods*, %struct.PyNumberMethods** %tmp26		; <%struct.PyNumberMethods*> [#uses=1]
	%tmp28 = getelementptr %struct.PyNumberMethods, %struct.PyNumberMethods* %tmp27, i32 0, i32 5		; <%struct.PyObject* (%struct.PyObject*, %struct.PyObject*)**> [#uses=1]
	%tmp29 = load %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*, %struct.PyObject* (%struct.PyObject*, %struct.PyObject*)** %tmp28		; <%struct.PyObject* (%struct.PyObject*, %struct.PyObject*)*> [#uses=1]
	%tmp32 = call %struct.PyObject* %tmp29( %struct.PyObject* %a, %struct.PyObject* %b )		; <%struct.PyObject*> [#uses=1]
	ret %struct.PyObject* %tmp32

bb35:		; preds = %_ubyte_convert2_to_ctypes.exit, %cond_next.i
	%tmp36 = load i8**, i8*** @PyUFunc_API, align 8		; <i8**> [#uses=1]
	%tmp37 = getelementptr i8*, i8** %tmp36, i64 27		; <i8**> [#uses=1]
	%tmp38 = load i8*, i8** %tmp37		; <i8*> [#uses=1]
	%tmp3839 = bitcast i8* %tmp38 to void ()*		; <void ()*> [#uses=1]
	call void %tmp3839( )
	%tmp40 = load i8, i8* %arg2, align 1		; <i8> [#uses=4]
	%tmp1.i = icmp eq i8 %tmp40, 0		; <i1> [#uses=2]
	br i1 %tmp1.i, label %cond_true.i, label %cond_false.i

cond_true.i:		; preds = %bb35
	%tmp3.i196 = call i32 @feraiseexcept( i32 4 )		; <i32> [#uses=0]
	%tmp46207 = load i8, i8* %arg2, align 1		; <i8> [#uses=3]
	%tmp48208 = load i8, i8* %arg1, align 1		; <i8> [#uses=2]
	%tmp1.i197210 = icmp eq i8 %tmp48208, 0		; <i1> [#uses=1]
	%tmp4.i212 = icmp eq i8 %tmp46207, 0		; <i1> [#uses=1]
	%tmp7.i198213 = or i1 %tmp1.i197210, %tmp4.i212		; <i1> [#uses=1]
	br i1 %tmp7.i198213, label %cond_true.i200, label %cond_next17.i

cond_false.i:		; preds = %bb35
	%tmp42 = load i8, i8* %arg1, align 1		; <i8> [#uses=3]
	%tmp7.i = udiv i8 %tmp42, %tmp40		; <i8> [#uses=2]
	%tmp1.i197 = icmp eq i8 %tmp42, 0		; <i1> [#uses=1]
	%tmp7.i198 = or i1 %tmp1.i197, %tmp1.i		; <i1> [#uses=1]
	br i1 %tmp7.i198, label %cond_true.i200, label %cond_next17.i

cond_true.i200:		; preds = %cond_false.i, %cond_true.i
	%out.0 = phi i8 [ 0, %cond_true.i ], [ %tmp7.i, %cond_false.i ]		; <i8> [#uses=2]
	%tmp46202.0 = phi i8 [ %tmp46207, %cond_true.i ], [ %tmp40, %cond_false.i ]		; <i8> [#uses=1]
	%tmp11.i199 = icmp eq i8 %tmp46202.0, 0		; <i1> [#uses=1]
	br i1 %tmp11.i199, label %cond_true14.i, label %ubyte_ctype_remainder.exit

cond_true14.i:		; preds = %cond_true.i200
	%tmp15.i = call i32 @feraiseexcept( i32 4 )		; <i32> [#uses=0]
	br label %ubyte_ctype_remainder.exit

cond_next17.i:		; preds = %cond_false.i, %cond_true.i
	%out.1 = phi i8 [ 0, %cond_true.i ], [ %tmp7.i, %cond_false.i ]		; <i8> [#uses=1]
	%tmp46202.1 = phi i8 [ %tmp46207, %cond_true.i ], [ %tmp40, %cond_false.i ]		; <i8> [#uses=1]
	%tmp48205.1 = phi i8 [ %tmp48208, %cond_true.i ], [ %tmp42, %cond_false.i ]		; <i8> [#uses=1]
	%tmp20.i = urem i8 %tmp48205.1, %tmp46202.1		; <i8> [#uses=1]
	br label %ubyte_ctype_remainder.exit

ubyte_ctype_remainder.exit:		; preds = %cond_next17.i, %cond_true14.i, %cond_true.i200
	%out2.0 = phi i8 [ %tmp20.i, %cond_next17.i ], [ 0, %cond_true14.i ], [ 0, %cond_true.i200 ]		; <i8> [#uses=1]
	%out.2 = phi i8 [ %out.1, %cond_next17.i ], [ %out.0, %cond_true14.i ], [ %out.0, %cond_true.i200 ]		; <i8> [#uses=1]
	%tmp52 = load i8**, i8*** @PyUFunc_API, align 8		; <i8**> [#uses=1]
	%tmp53 = getelementptr i8*, i8** %tmp52, i64 28		; <i8**> [#uses=1]
	%tmp54 = load i8*, i8** %tmp53		; <i8*> [#uses=1]
	%tmp5455 = bitcast i8* %tmp54 to i32 ()*		; <i32 ()*> [#uses=1]
	%tmp56 = call i32 %tmp5455( )		; <i32> [#uses=2]
	%tmp58 = icmp eq i32 %tmp56, 0		; <i1> [#uses=1]
	br i1 %tmp58, label %cond_next89, label %cond_true61

cond_true61:		; preds = %ubyte_ctype_remainder.exit
	%tmp62 = load i8**, i8*** @PyUFunc_API, align 8		; <i8**> [#uses=1]
	%tmp63 = getelementptr i8*, i8** %tmp62, i64 25		; <i8**> [#uses=1]
	%tmp64 = load i8*, i8** %tmp63		; <i8*> [#uses=1]
	%tmp6465 = bitcast i8* %tmp64 to i32 (i8*, i32*, i32*, %struct.PyObject**)*		; <i32 (i8*, i32*, i32*, %struct.PyObject**)*> [#uses=1]
	%tmp67 = call i32 %tmp6465( i8* getelementptr ([14 x i8]* @.str5, i32 0, i64 0), i32* %bufsize, i32* %errmask, %struct.PyObject** %errobj )		; <i32> [#uses=1]
	%tmp68 = icmp slt i32 %tmp67, 0		; <i1> [#uses=1]
	br i1 %tmp68, label %UnifiedReturnBlock, label %cond_next73

cond_next73:		; preds = %cond_true61
	store i32 1, i32* %first, align 4
	%tmp74 = load i8**, i8*** @PyUFunc_API, align 8		; <i8**> [#uses=1]
	%tmp75 = getelementptr i8*, i8** %tmp74, i64 29		; <i8**> [#uses=1]
	%tmp76 = load i8*, i8** %tmp75		; <i8*> [#uses=1]
	%tmp7677 = bitcast i8* %tmp76 to i32 (i32, %struct.PyObject*, i32, i32*)*		; <i32 (i32, %struct.PyObject*, i32, i32*)*> [#uses=1]
	%tmp79 = load %struct.PyObject*, %struct.PyObject** %errobj, align 8		; <%struct.PyObject*> [#uses=1]
	%tmp80 = load i32, i32* %errmask, align 4		; <i32> [#uses=1]
	%tmp82 = call i32 %tmp7677( i32 %tmp80, %struct.PyObject* %tmp79, i32 %tmp56, i32* %first )		; <i32> [#uses=1]
	%tmp83 = icmp eq i32 %tmp82, 0		; <i1> [#uses=1]
	br i1 %tmp83, label %cond_next89, label %UnifiedReturnBlock

cond_next89:		; preds = %cond_next73, %ubyte_ctype_remainder.exit
	%tmp90 = call %struct.PyObject* @PyTuple_New( i64 2 )		; <%struct.PyObject*> [#uses=9]
	%tmp92 = icmp eq %struct.PyObject* %tmp90, null		; <i1> [#uses=1]
	br i1 %tmp92, label %UnifiedReturnBlock, label %cond_next97

cond_next97:		; preds = %cond_next89
	%tmp98 = load i8**, i8*** @PyArray_API, align 8		; <i8**> [#uses=1]
	%tmp99 = getelementptr i8*, i8** %tmp98, i64 25		; <i8**> [#uses=1]
	%tmp100 = load i8*, i8** %tmp99		; <i8*> [#uses=1]
	%tmp100101 = bitcast i8* %tmp100 to %struct._typeobject*		; <%struct._typeobject*> [#uses=2]
	%tmp102 = getelementptr %struct._typeobject, %struct._typeobject* %tmp100101, i32 0, i32 38		; <%struct.PyObject* (%struct._typeobject*, i64)**> [#uses=1]
	%tmp103 = load %struct.PyObject* (%struct._typeobject*, i64)*, %struct.PyObject* (%struct._typeobject*, i64)** %tmp102		; <%struct.PyObject* (%struct._typeobject*, i64)*> [#uses=1]
	%tmp108 = call %struct.PyObject* %tmp103( %struct._typeobject* %tmp100101, i64 0 )		; <%struct.PyObject*> [#uses=3]
	%tmp110 = icmp eq %struct.PyObject* %tmp108, null		; <i1> [#uses=1]
	br i1 %tmp110, label %cond_true113, label %cond_next135

cond_true113:		; preds = %cond_next97
	%tmp115 = getelementptr %struct.PyObject, %struct.PyObject* %tmp90, i32 0, i32 0		; <i64*> [#uses=2]
	%tmp116 = load i64, i64* %tmp115		; <i64> [#uses=1]
	%tmp117 = add i64 %tmp116, -1		; <i64> [#uses=2]
	store i64 %tmp117, i64* %tmp115
	%tmp123 = icmp eq i64 %tmp117, 0		; <i1> [#uses=1]
	br i1 %tmp123, label %cond_true126, label %UnifiedReturnBlock

cond_true126:		; preds = %cond_true113
	%tmp128 = getelementptr %struct.PyObject, %struct.PyObject* %tmp90, i32 0, i32 1		; <%struct._typeobject**> [#uses=1]
	%tmp129 = load %struct._typeobject*, %struct._typeobject** %tmp128		; <%struct._typeobject*> [#uses=1]
	%tmp130 = getelementptr %struct._typeobject, %struct._typeobject* %tmp129, i32 0, i32 6		; <void (%struct.PyObject*)**> [#uses=1]
	%tmp131 = load void (%struct.PyObject*)*, void (%struct.PyObject*)** %tmp130		; <void (%struct.PyObject*)*> [#uses=1]
	call void %tmp131( %struct.PyObject* %tmp90 )
	ret %struct.PyObject* null

cond_next135:		; preds = %cond_next97
	%tmp136137 = bitcast %struct.PyObject* %tmp108 to %struct.PyBoolScalarObject*		; <%struct.PyBoolScalarObject*> [#uses=1]
	%tmp139 = getelementptr %struct.PyBoolScalarObject, %struct.PyBoolScalarObject* %tmp136137, i32 0, i32 2		; <i8*> [#uses=1]
	store i8 %out.2, i8* %tmp139
	%tmp140141 = bitcast %struct.PyObject* %tmp90 to %struct.PyTupleObject*		; <%struct.PyTupleObject*> [#uses=2]
	%tmp143 = getelementptr %struct.PyTupleObject, %struct.PyTupleObject* %tmp140141, i32 0, i32 3, i64 0		; <%struct.PyObject**> [#uses=1]
	store %struct.PyObject* %tmp108, %struct.PyObject** %tmp143
	%tmp145 = load i8**, i8*** @PyArray_API, align 8		; <i8**> [#uses=1]
	%tmp146 = getelementptr i8*, i8** %tmp145, i64 25		; <i8**> [#uses=1]
	%tmp147 = load i8*, i8** %tmp146		; <i8*> [#uses=1]
	%tmp147148 = bitcast i8* %tmp147 to %struct._typeobject*		; <%struct._typeobject*> [#uses=2]
	%tmp149 = getelementptr %struct._typeobject, %struct._typeobject* %tmp147148, i32 0, i32 38		; <%struct.PyObject* (%struct._typeobject*, i64)**> [#uses=1]
	%tmp150 = load %struct.PyObject* (%struct._typeobject*, i64)*, %struct.PyObject* (%struct._typeobject*, i64)** %tmp149		; <%struct.PyObject* (%struct._typeobject*, i64)*> [#uses=1]
	%tmp155 = call %struct.PyObject* %tmp150( %struct._typeobject* %tmp147148, i64 0 )		; <%struct.PyObject*> [#uses=3]
	%tmp157 = icmp eq %struct.PyObject* %tmp155, null		; <i1> [#uses=1]
	br i1 %tmp157, label %cond_true160, label %cond_next182

cond_true160:		; preds = %cond_next135
	%tmp162 = getelementptr %struct.PyObject, %struct.PyObject* %tmp90, i32 0, i32 0		; <i64*> [#uses=2]
	%tmp163 = load i64, i64* %tmp162		; <i64> [#uses=1]
	%tmp164 = add i64 %tmp163, -1		; <i64> [#uses=2]
	store i64 %tmp164, i64* %tmp162
	%tmp170 = icmp eq i64 %tmp164, 0		; <i1> [#uses=1]
	br i1 %tmp170, label %cond_true173, label %UnifiedReturnBlock

cond_true173:		; preds = %cond_true160
	%tmp175 = getelementptr %struct.PyObject, %struct.PyObject* %tmp90, i32 0, i32 1		; <%struct._typeobject**> [#uses=1]
	%tmp176 = load %struct._typeobject*, %struct._typeobject** %tmp175		; <%struct._typeobject*> [#uses=1]
	%tmp177 = getelementptr %struct._typeobject, %struct._typeobject* %tmp176, i32 0, i32 6		; <void (%struct.PyObject*)**> [#uses=1]
	%tmp178 = load void (%struct.PyObject*)*, void (%struct.PyObject*)** %tmp177		; <void (%struct.PyObject*)*> [#uses=1]
	call void %tmp178( %struct.PyObject* %tmp90 )
	ret %struct.PyObject* null

cond_next182:		; preds = %cond_next135
	%tmp183184 = bitcast %struct.PyObject* %tmp155 to %struct.PyBoolScalarObject*		; <%struct.PyBoolScalarObject*> [#uses=1]
	%tmp186 = getelementptr %struct.PyBoolScalarObject, %struct.PyBoolScalarObject* %tmp183184, i32 0, i32 2		; <i8*> [#uses=1]
	store i8 %out2.0, i8* %tmp186
	%tmp190 = getelementptr %struct.PyTupleObject, %struct.PyTupleObject* %tmp140141, i32 0, i32 3, i64 1		; <%struct.PyObject**> [#uses=1]
	store %struct.PyObject* %tmp155, %struct.PyObject** %tmp190
	ret %struct.PyObject* %tmp90

UnifiedReturnBlock:		; preds = %cond_true160, %cond_true113, %cond_next89, %cond_next73, %cond_true61, %bb17
	ret %struct.PyObject* null
}

declare i32 @feraiseexcept(i32)

declare fastcc i32 @_ubyte_convert_to_ctype(%struct.PyObject*, i8*)

declare %struct.PyObject* @PyErr_Occurred()

declare %struct.PyObject* @PyTuple_New(i64)
