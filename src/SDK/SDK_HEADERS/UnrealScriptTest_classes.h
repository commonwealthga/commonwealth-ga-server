/*
#############################################################################################
# Global Agenda (1.5.1.5) SDK
# Generated with TheFeckless UE3 SDK Generator v1.4_Beta-Rev.51
# ========================================================================================= #
# File: UnrealScriptTest_classes.h
# ========================================================================================= #
# Credits: uNrEaL, Tamimego, SystemFiles, R00T88, _silencer, the1domo, K@N@VEL
# Thanks: HOOAH07, lowHertz
# Forums: www.uc-forum.com, www.gamedeception.net
#############################################################################################
*/

#ifdef _MSC_VER
	#pragma pack ( push, 0x4 )
#endif

/*
# ========================================================================================= #
# Constants
# ========================================================================================= #
*/

#define CONST_ExampleConst                                       1.f

/*
# ========================================================================================= #
# Enums
# ========================================================================================= #
*/

// Enum UnrealScriptTest.Test0012_IfVarEqualEqualEnum.Test0012_SomeEnumFoo
/*enum Test0012_SomeEnumFoo
{
	Test0012SEF_ValOne                                 = 0,
	Test0012SEF_ValTwo                                 = 1,
	Test0012SEF_MAX                                    = 2
};*/

// Enum UnrealScriptTest.Test0013_IfEnumEqualEqualVar.Test0013_SomeEnumFoo
/*enum Test0013_SomeEnumFoo
{
	Test0013SEF_ValOne                                 = 0,
	Test0013SEF_ValTwo                                 = 1,
	Test0013SEF_MAX                                    = 2
};*/

// Enum UnrealScriptTest.Test0013_IfEnumEqualEqualVar.Test0013_AnotherEnumFoo
/*enum Test0013_AnotherEnumFoo
{
	Test0013AEF_ValOne                                 = 0,
	Test0013AEF_ValTwo                                 = 1,
	Test0013AEF_MAX                                    = 2
};*/

// Enum UnrealScriptTest.Test0013_IfEnumEqualEqualVar.ENetRole
/*enum ENetRole
{
	ROLE_None                                          = 0,
	ROLE_SimulatedProxy                                = 1,
	ROLE_AutonomousProxy                               = 2,
	ROLE_Authority                                     = 3,
	ROLE_MAX                                           = 4
};*/

// Enum UnrealScriptTest.Test0013_IfEnumEqualEqualVar.ECollisionType
/*enum ECollisionType
{
	COLLIDE_CustomDefault                              = 0,
	COLLIDE_NoCollision                                = 1,
	COLLIDE_BlockAll                                   = 2,
	COLLIDE_MAX                                        = 3
};*/

// Enum UnrealScriptTest.Test0014_FullyQualifiedEnumFunction.Test14_SomeEnumFoo
/*enum Test14_SomeEnumFoo
{
	Test14SEF_ValOne                                   = 0,
	Test14SEF_ValTwo                                   = 1,
	Test14SEF_MAX                                      = 2
};*/

// Enum UnrealScriptTest.Test0015_FullyQualifiedEnumSwitchCase.Test15_SomeEnumFoo
/*enum Test15_SomeEnumFoo
{
	Test15SEF_ValOne                                   = 0,
	Test15SEF_ValTwo                                   = 1,
	Test15SEF_MAX                                      = 2
};*/

// Enum UnrealScriptTest.Test0023_MetaData.EMetadataTestEnum
/*enum EMetadataTestEnum
{
	METADATA_FirstValue                                = 0,
	METADATA_SecondValue                               = 1,
	METADATA_ThirdValue                                = 2,
	METADATA_MAX                                       = 3
};*/

// Enum UnrealScriptTest.Test0025_NativeIssues.EBoolOrderEnum
/*enum EBoolOrderEnum
{
	BOOLORDER_One                                      = 0,
	BOOLORDER_Two                                      = 1,
	BOOLORDER_Three                                    = 2,
	BOOLORDER_MAX                                      = 3
};*/


/*
# ========================================================================================= #
# Classes
# ========================================================================================= #
*/

// Class UnrealScriptTest.ByteCodeSerializer
// 0x0044 (0x00D4 - 0x0090)
class UByteCodeSerializer : public UStruct
{
public:
	unsigned char                                      UnknownData00[ 0x44 ];                            		// 0x0090 (0x0044) MISSED OFFSET

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1172 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.ComponentTestActorBase
// 0x0000 (0x01CC - 0x01CC)
class AComponentTestActorBase : public AActor
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1173 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.CTComplexNestRoot
// 0x0014 (0x01E0 - 0x01CC)
class ACTComplexNestRoot : public AComponentTestActorBase
{
public:
	class UCTSimpleNestObject*                         SimpleNestObject;                                 		// 0x01CC (0x0004) [0x0000000004400009]              ( CPF_Edit | CPF_ExportObject | CPF_NeedCtorLink | CPF_EditInline )
	struct FComplexNestStruct                          SimpleNestObjectStruct;                           		// 0x01D0 (0x0010) [0x0000000004400009]              ( CPF_Edit | CPF_ExportObject | CPF_NeedCtorLink | CPF_EditInline )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1174 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.CTInheritTestRoot
// 0x0000 (0x01CC - 0x01CC)
class ACTInheritTestRoot : public AComponentTestActorBase
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1175 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.CTNoNestRoot
// 0x0028 (0x01F4 - 0x01CC)
class ACTNoNestRoot : public AComponentTestActorBase
{
public:
	class UCTNoNesting*                                UnmodifiedComponent_SingleRef;                    		// 0x01CC (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTNoNesting*                                ModifiedComponent_SingleRef;                      		// 0x01D0 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTNoNesting*                                UnmodifiedComponent_MultipleRef;                  		// 0x01D4 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTNoNesting*                                ModifiedComponent_MultipleRef;                    		// 0x01D8 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	TArray< class UCTNoNesting* >                      UnmodifiedComponent_Array;                        		// 0x01DC (0x000C) [0x0000000004480009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_NeedCtorLink | CPF_EditInline )
	TArray< class UCTNoNesting* >                      ModifiedComponent_Array;                          		// 0x01E8 (0x000C) [0x0000000004480009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_NeedCtorLink | CPF_EditInline )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1176 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.CTSimpleNestRoot
// 0x0028 (0x01F4 - 0x01CC)
class ACTSimpleNestRoot : public AComponentTestActorBase
{
public:
	class UCTSimpleNestParent*                         UnmodifiedNestParent_SingleRef;                   		// 0x01CC (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTSimpleNestParent*                         ModifiedNestParent_SingleRef;                     		// 0x01D0 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTSimpleNestParent*                         UnmodifiedNestParent_MultipleRef;                 		// 0x01D4 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTSimpleNestParent*                         ModifiedNestParent_MultipleRef;                   		// 0x01D8 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	TArray< class UCTSimpleNestParent* >               UnmodifiedNestParent_Array;                       		// 0x01DC (0x000C) [0x0000000004480009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_NeedCtorLink | CPF_EditInline )
	TArray< class UCTSimpleNestParent* >               ModifiedNestParent_Array;                         		// 0x01E8 (0x000C) [0x0000000004480009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_NeedCtorLink | CPF_EditInline )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1177 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.FindUnreferencedFunctionsCommandlet
// 0x0004 (0x0080 - 0x007C)
class UFindUnreferencedFunctionsCommandlet : public UCommandlet
{
public:
	class UByteCodeSerializer*                         Serializer;                                       		// 0x007C (0x0004) [0x0000000000002002]              ( CPF_Const | CPF_Transient )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1178 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.Test0002_InterfaceNative
// 0x0000 (0x003C - 0x003C)
class UTest0002_InterfaceNative : public UInterface
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1179 ];

		return pClassPointer;
	};

	void TestObjectToInterfaceConversions ( );
	void eventTestInterfaceEvent ( class UObject* ObjParam );
	void TestNativeFunction ( unsigned long bBoolParm );
};


// Class UnrealScriptTest.Test0010_NativeObject
// 0x006C (0x00A8 - 0x003C)
class UTest0010_NativeObject : public UObject
{
public:
	struct FPointer                                    VfTable_ITest0002_InterfaceNative;                		// 0x003C (0x0004) [0x0000000000801002]              ( CPF_Const | CPF_Native | CPF_NoExport )
	class UTest0002_InterfaceNative*                   InterfaceMember;                                  		// 0x0040 (0x0008) [0x0000000000000000]              
	unsigned char                                      UnknownData00[ 0x4 ];                             		// 0x0044 (0x0004) FIX WRONG TYPE SIZE OF PREVIUS PROPERTY
	TArray< class UTest0002_InterfaceNative* >         InterfaceArray;                                   		// 0x0048 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FConstructorStructString                    DefaultStringStruct;                              		// 0x0054 (0x0018) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FConstructorStructArray                     DefaultArrayStruct;                               		// 0x006C (0x0010) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FConstructorStructCombo                     DefaultComboStruct;                               		// 0x007C (0x0020) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct UTest0010_NativeObject_FMyStruct >  MyArray;                                          		// 0x009C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1180 ];

		return pClassPointer;
	};

	void eventTestOptionalEventStringParm ( struct FString FirstString, struct FString SecondString );
	void FunctionWithMultipleOptionalParms ( struct FString FirstString, struct FString SecondString );
	void eventTestMultipleOptionalParms ( );
	void VerifyConversionFromInterfaceToObjectAsNativeParm ( class UObject* InObject, int DummyInt );
	void TestInterfaceObject_OutParmCompatibility ( class UObject** out_Object, class UTest0002_InterfaceNative** out_Interface );
	void eventTest06_InterfaceToObjectConversions ( );
	void eventTest05_StructInheritance ( );
	void TestObjectToInterfaceConversions ( );
	void eventTestInterfaceEvent ( class UObject* ObjParam );
	void TestNativeFunction ( unsigned long bBoolParm );
	void eventTest03_CallEventWithNativeInterface ( class UTest0002_InterfaceNative* InterfaceParm );
	void Test02_PassNativeInterfaceToNativeFunction ( class UTest0002_InterfaceNative* InterfaceParm );
	void eventTest01_CallEventWithStruct ( struct FNoCtorProps NoCtor, struct FConstructorStructString StringParm, struct FConstructorStructArray ArrayParm, struct FConstructorStructCombo ComboParm, unsigned long PaddingBool );
	void PerformNativeTest ( int TestNumber );
};


// Class UnrealScriptTest.Test0011_NativeObjectBoolOrder
// 0x0004 (0x0040 - 0x003C)
class UTest0011_NativeObjectBoolOrder : public UObject
{
public:
	unsigned long                                      bMemberBoolA : 1;                                 		// 0x003C (0x0004) [0x0000000000000001] [0x00000001] ( CPF_Edit )
	unsigned long                                      bMemberBoolB : 1;                                 		// 0x003C (0x0004) [0x0000000000000001] [0x00000002] ( CPF_Edit )
	unsigned long                                      bMemberBoolC : 1;                                 		// 0x003C (0x0004) [0x0000000000000001] [0x00000004] ( CPF_Edit )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1181 ];

		return pClassPointer;
	};

	void eventNativeTestBoolOrderEvent ( unsigned long EventBoolParm );
	bool NativeTestBoolOrderFunction ( unsigned long BoolParm );
	void PerformBoolOrderTest ( );
};


// Class UnrealScriptTest.TestClassBase
// 0x0000 (0x003C - 0x003C)
class UTestClassBase : public UObject
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1182 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.CTSimpleNestObject
// 0x0004 (0x0040 - 0x003C)
class UCTSimpleNestObject : public UTestClassBase
{
public:
	class UCTSimpleNestChild*                          InnerComponent;                                   		// 0x003C (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1183 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.Test0025_NativeIssues
// 0x0008 (0x0044 - 0x003C)
class UTest0025_NativeIssues : public UTestClassBase
{
public:
	unsigned char                                      EnumBeforeBoolTestVar;                            		// 0x003C (0x0001) [0x0000000000000000]              
	unsigned long                                      bBoolAfterEnumTestVar : 1;                        		// 0x0040 (0x0004) [0x0000000000000000] [0x00000001] 

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1184 ];

		return pClassPointer;
	};

	void RunTest ( );
	void ShowNativeBoolAssignmentTestResults ( );
};


// Class UnrealScriptTest.TestComponentsBase
// 0x000B (0x0060 - 0x0055)
class UTestComponentsBase : public UActorComponent
{
public:
	int                                                UnmodifiedInt;                                    		// 0x0058 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              ModifiedFloat;                                    		// 0x005C (0x0004) [0x0000000000000001]              ( CPF_Edit )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1185 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.CTComplexNestChild
// 0x0050 (0x00B0 - 0x0060)
class UCTComplexNestChild : public UTestComponentsBase
{
public:
	class UCTComplexNestGrandchild*                    UnmodifiedGrandChild_Defined_SingleRef;           		// 0x0060 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTComplexNestGrandchild*                    UnmodifiedGrandChild_Defined_MultipleRef;         		// 0x0064 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTComplexNestGrandchild*                    ModifiedGrandChild_Defined_SingleRef;             		// 0x0068 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTComplexNestGrandchild*                    ModifiedGrandChild_Defined_MultipleRef;           		// 0x006C (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTComplexNestGrandchild*                    UnmodifiedGrandChild_NotDefined_SingleRef;        		// 0x0070 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTComplexNestGrandchild*                    UnmodifiedGrandChild_NotDefined_MultipleRef;      		// 0x0074 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTComplexNestGrandchild*                    ModifiedGrandChild_NotDefined_SingleRef;          		// 0x0078 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTComplexNestGrandchild*                    ModifiedGrandChild_NotDefined_MultipleRef;        		// 0x007C (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	TArray< class UCTComplexNestGrandchild* >          UnmodifiedGrandChild_Defined_Array;               		// 0x0080 (0x000C) [0x0000000004480009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_NeedCtorLink | CPF_EditInline )
	TArray< class UCTComplexNestGrandchild* >          ModifiedGrandChild_Defined_Array;                 		// 0x008C (0x000C) [0x0000000004480009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_NeedCtorLink | CPF_EditInline )
	TArray< class UCTComplexNestGrandchild* >          UnmodifiedGrandChild_Undefined_Array;             		// 0x0098 (0x000C) [0x0000000004480009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_NeedCtorLink | CPF_EditInline )
	TArray< class UCTComplexNestGrandchild* >          ModifiedGrandChild_Undefined_Array;               		// 0x00A4 (0x000C) [0x0000000004480009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_NeedCtorLink | CPF_EditInline )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1186 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.CTComplexNestGrandchild
// 0x0000 (0x0060 - 0x0060)
class UCTComplexNestGrandchild : public UTestComponentsBase
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1187 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.CTComplexNestParent
// 0x0050 (0x00B0 - 0x0060)
class UCTComplexNestParent : public UTestComponentsBase
{
public:
	class UCTComplexNestChild*                         UnmodifiedChild_Defined_SingleRef;                		// 0x0060 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTComplexNestChild*                         UnmodifiedChild_Defined_MultipleRef;              		// 0x0064 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTComplexNestChild*                         ModifiedChild_Defined_SingleRef;                  		// 0x0068 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTComplexNestChild*                         ModifiedChild_Defined_MultipleRef;                		// 0x006C (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTComplexNestChild*                         UnmodifiedChild_NotDefined_SingleRef;             		// 0x0070 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTComplexNestChild*                         UnmodifiedChild_NotDefined_MultipleRef;           		// 0x0074 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTComplexNestChild*                         ModifiedChild_NotDefined_SingleRef;               		// 0x0078 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTComplexNestChild*                         ModifiedChild_NotDefined_MultipleRef;             		// 0x007C (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	TArray< class UCTComplexNestChild* >               UnmodifiedChild_Defined_Array;                    		// 0x0080 (0x000C) [0x0000000004480009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_NeedCtorLink | CPF_EditInline )
	TArray< class UCTComplexNestChild* >               ModifiedChild_Defined_Array;                      		// 0x008C (0x000C) [0x0000000004480009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_NeedCtorLink | CPF_EditInline )
	TArray< class UCTComplexNestChild* >               UnmodifiedChild_Undefined_Array;                  		// 0x0098 (0x000C) [0x0000000004480009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_NeedCtorLink | CPF_EditInline )
	TArray< class UCTComplexNestChild* >               ModifiedChild_Undefined_Array;                    		// 0x00A4 (0x000C) [0x0000000004480009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_NeedCtorLink | CPF_EditInline )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1188 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.CTNoNesting
// 0x0000 (0x0060 - 0x0060)
class UCTNoNesting : public UTestComponentsBase
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1189 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.CTSimpleNestChild
// 0x0000 (0x0060 - 0x0060)
class UCTSimpleNestChild : public UTestComponentsBase
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1190 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.CTSimpleNestParent
// 0x0050 (0x00B0 - 0x0060)
class UCTSimpleNestParent : public UTestComponentsBase
{
public:
	class UCTSimpleNestChild*                          UnmodifiedChild_Defined_SingleRef;                		// 0x0060 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTSimpleNestChild*                          UnmodifiedChild_Defined_MultipleRef;              		// 0x0064 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTSimpleNestChild*                          ModifiedChild_Defined_SingleRef;                  		// 0x0068 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTSimpleNestChild*                          ModifiedChild_Defined_MultipleRef;                		// 0x006C (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTSimpleNestChild*                          UnmodifiedChild_NotDefined_SingleRef;             		// 0x0070 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTSimpleNestChild*                          UnmodifiedChild_NotDefined_MultipleRef;           		// 0x0074 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTSimpleNestChild*                          ModifiedChild_NotDefined_SingleRef;               		// 0x0078 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UCTSimpleNestChild*                          ModifiedChild_NotDefined_MultipleRef;             		// 0x007C (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	TArray< class UCTSimpleNestChild* >                UnmodifiedChild_Defined_Array;                    		// 0x0080 (0x000C) [0x0000000004480009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_NeedCtorLink | CPF_EditInline )
	TArray< class UCTSimpleNestChild* >                ModifiedChild_Defined_Array;                      		// 0x008C (0x000C) [0x0000000004480009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_NeedCtorLink | CPF_EditInline )
	TArray< class UCTSimpleNestChild* >                UnmodifiedChild_Undefined_Array;                  		// 0x0098 (0x000C) [0x0000000004480009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_NeedCtorLink | CPF_EditInline )
	TArray< class UCTSimpleNestChild* >                ModifiedChild_Undefined_Array;                    		// 0x00A4 (0x000C) [0x0000000004480009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_NeedCtorLink | CPF_EditInline )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1191 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.NestingTest_SecondLevelComponent
// 0x0004 (0x0064 - 0x0060)
class UNestingTest_SecondLevelComponent : public UTestComponentsBase
{
public:
	float                                              TestFloat;                                        		// 0x0060 (0x0004) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30484 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.InheritanceTestBase
// 0x0008 (0x0068 - 0x0060)
class UInheritanceTestBase : public UTestComponentsBase
{
public:
	int                                                TestInt;                                          		// 0x0060 (0x0004) [0x0000000000020001]              ( CPF_Edit | CPF_EditConst )
	class UNestingTest_SecondLevelComponent*           TestComponent;                                    		// 0x0064 (0x0004) [0x00000000040A0009]              ( CPF_Edit | CPF_ExportObject | CPF_EditConst | CPF_Component | CPF_EditInline )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30487 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.InheritanceTestDerived
// 0x0000 (0x0068 - 0x0068)
class UInheritanceTestDerived : public UInheritanceTestBase
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30492 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.NestingTest_SecondLevelSubobject
// 0x0004 (0x0040 - 0x003C)
class UNestingTest_SecondLevelSubobject : public UTestClassBase
{
public:
	struct FColor                                      TestColor;                                        		// 0x003C (0x0004) [0x0000000000000001]              ( CPF_Edit )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30495 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.NestingTest_FirstLevelComponent
// 0x0008 (0x0068 - 0x0060)
class UNestingTest_FirstLevelComponent : public UTestComponentsBase
{
public:
	class UNestingTest_SecondLevelSubobject*           NestedSubobject;                                  		// 0x0060 (0x0004) [0x0000000004400008]              ( CPF_ExportObject | CPF_NeedCtorLink | CPF_EditInline )
	class UNestingTest_SecondLevelComponent*           NestedComponent;                                  		// 0x0064 (0x0004) [0x0000000004080008]              ( CPF_ExportObject | CPF_Component | CPF_EditInline )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30498 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.NestingTest_FirstLevelSubobject
// 0x0018 (0x0054 - 0x003C)
class UNestingTest_FirstLevelSubobject : public UTestClassBase
{
public:
	class UNestingTest_SecondLevelSubobject*           NestedSubobject;                                  		// 0x003C (0x0004) [0x0000000004400009]              ( CPF_Edit | CPF_ExportObject | CPF_NeedCtorLink | CPF_EditInline )
	class UNestingTest_SecondLevelComponent*           NestedComponent;                                  		// 0x0040 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	struct FNestedObjectStruct                         NestedStruct;                                     		// 0x0044 (0x0010) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30504 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.SubobjectTestActor
// 0x0008 (0x01D4 - 0x01CC)
class ASubobjectTestActor : public AActor
{
public:
	class UNestingTest_FirstLevelComponent*            Component;                                        		// 0x01CC (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	class UNestingTest_FirstLevelSubobject*            Subobject;                                        		// 0x01D0 (0x0004) [0x0000000004400009]              ( CPF_Edit | CPF_ExportObject | CPF_NeedCtorLink | CPF_EditInline )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30515 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.Test0000_Interface
// 0x0000 (0x003C - 0x003C)
class UTest0000_Interface : public UInterface
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30521 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.Test0001_InterfaceBase
// 0x0000 (0x003C - 0x003C)
class UTest0001_InterfaceBase : public UInterface
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30523 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.Test0001_InterfaceDerived
// 0x0000 (0x003C - 0x003C)
class UTest0001_InterfaceDerived : public UTest0001_InterfaceBase
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30525 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.Test0003_BaseClass
// 0x0000 (0x003C - 0x003C)
class UTest0003_BaseClass : public UObject
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30532 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.TestCommandletBase
// 0x0000 (0x007C - 0x007C)
class UTestCommandletBase : public UCommandlet
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30534 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.Test0003_Commandlet
// 0x0000 (0x007C - 0x007C)
class UTest0003_Commandlet : public UTestCommandletBase
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30536 ];

		return pClassPointer;
	};

	int Test0003 ( );
	int eventMain ( struct FString Params );
};


// Class UnrealScriptTest.Test0003_Interface
// 0x0000 (0x003C - 0x003C)
class UTest0003_Interface : public UInterface
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30543 ];

		return pClassPointer;
	};

	unsigned char Test0003_FunctionF ( );
	float Test0003_FunctionE ( );
	int Test0003_FunctionD ( );
	struct FString Test0003_FunctionC ( );
	bool Test0003_FunctionB ( );
	void Test0003_FunctionA ( );
};


// Class UnrealScriptTest.Test0003_DerivedClass
// 0x0000 (0x003C - 0x003C)
class UTest0003_DerivedClass : public UTest0003_BaseClass
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30545 ];

		return pClassPointer;
	};

	unsigned char Test0003_FunctionF ( );
	float Test0003_FunctionE ( );
	int Test0003_FunctionD ( );
	struct FString Test0003_FunctionC ( );
	bool Test0003_FunctionB ( );
	void Test0003_FunctionA ( );
};


// Class UnrealScriptTest.Test0004_BaseClass
// 0x0000 (0x003C - 0x003C)
class UTest0004_BaseClass : public UObject
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30573 ];

		return pClassPointer;
	};

	unsigned char Test0004_FunctionF ( );
	float Test0004_FunctionE ( );
	int Test0004_FunctionD ( );
	struct FString Test0004_FunctionC ( );
	bool Test0004_FunctionB ( );
	void Test0004_FunctionA ( );
};


// Class UnrealScriptTest.Test0004_Interface
// 0x0000 (0x003C - 0x003C)
class UTest0004_Interface : public UInterface
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30585 ];

		return pClassPointer;
	};

	unsigned char Test0004_FunctionF ( );
	float Test0004_FunctionE ( );
	int Test0004_FunctionD ( );
	struct FString Test0004_FunctionC ( );
	bool Test0004_FunctionB ( );
	void Test0004_FunctionA ( );
};


// Class UnrealScriptTest.Test0004_DerivedClass
// 0x0000 (0x003C - 0x003C)
class UTest0004_DerivedClass : public UTest0004_BaseClass
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30587 ];

		return pClassPointer;
	};

	void Test0004_FunctionA ( );
};


// Class UnrealScriptTest.Test0005_Commandlet
// 0x0000 (0x007C - 0x007C)
class UTest0005_Commandlet : public UTestCommandletBase
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30602 ];

		return pClassPointer;
	};

	int Test0005 ( );
	int eventMain ( struct FString Params );
};


// Class UnrealScriptTest.Test0005_ImplementingClass
// 0x0000 (0x003C - 0x003C)
class UTest0005_ImplementingClass : public UObject
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30609 ];

		return pClassPointer;
	};

	struct FString derivedFoo ( );
	struct FString baseFoo ( );
};


// Class UnrealScriptTest.Test0005_InterfaceBase
// 0x0000 (0x003C - 0x003C)
class UTest0005_InterfaceBase : public UInterface
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30612 ];

		return pClassPointer;
	};

	struct FString baseFoo ( );
};


// Class UnrealScriptTest.Test0005_InterfaceDerived
// 0x0000 (0x003C - 0x003C)
class UTest0005_InterfaceDerived : public UTest0005_InterfaceBase
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30611 ];

		return pClassPointer;
	};

	struct FString derivedFoo ( );
};


// Class UnrealScriptTest.Test0006_InterfaceWithVariables
// 0x0000 (0x003C - 0x003C)
class UTest0006_InterfaceWithVariables : public UInterface
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30628 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.Test0007_InterfaceWithWithinClass
// 0x0000 (0x003C - 0x003C)
class UTest0007_InterfaceWithWithinClass : public UInterface
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30630 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.Test0008_ClassWithAnVarInterface
// 0x0000 (0x003C - 0x003C)
class UTest0008_ClassWithAnVarInterface : public UObject
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30632 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.Test0009_DerivedClassWithPPInclude
// 0x0000 (0x003C - 0x003C)
class UTest0009_DerivedClassWithPPInclude : public UObject
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30634 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.Test0010_Commandlet
// 0x0000 (0x007C - 0x007C)
class UTest0010_Commandlet : public UTestCommandletBase
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30636 ];

		return pClassPointer;
	};

	void TestInterface ( );
	int Test0010 ( int TestNumber );
	int eventMain ( struct FString Params );
};


// Class UnrealScriptTest.Test0011_Commandlet
// 0x0000 (0x007C - 0x007C)
class UTest0011_Commandlet : public UTestCommandletBase
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30718 ];

		return pClassPointer;
	};

	void PerformBoolOrderTest ( );
	int eventMain ( struct FString Parms );
};


// Class UnrealScriptTest.Test0012_IfVarEqualEqualEnum
// 0x0000 (0x003C - 0x003C)
class UTest0012_IfVarEqualEqualEnum : public UObject
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30734 ];

		return pClassPointer;
	};

	void VarEqualEqualEnumTypeEnum ( );
	void VarEqualEqualEnum ( );
};


// Class UnrealScriptTest.Test0012_IfVarEqualEqualEnumDerivedClass
// 0x0000 (0x003C - 0x003C)
class UTest0012_IfVarEqualEqualEnumDerivedClass : public UTest0012_IfVarEqualEqualEnum
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30741 ];

		return pClassPointer;
	};

	void VarEqualEqualEnumTypeEnum ( );
	void VarEqualEqualEnum ( );
};


// Class UnrealScriptTest.Test0013_Commandlet
// 0x0000 (0x007C - 0x007C)
class UTest0013_Commandlet : public UTestCommandletBase
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30746 ];

		return pClassPointer;
	};

	int eventMain ( struct FString Parms );
};


// Class UnrealScriptTest.Test0013_IfEnumEqualEqualVar
// 0x0004 (0x0040 - 0x003C)
class UTest0013_IfEnumEqualEqualVar : public UObject
{
public:
	unsigned char                                      SomeFoo;                                          		// 0x003C (0x0001) [0x0000000000000000]              
	unsigned char                                      AnotherFoo;                                       		// 0x003D (0x0001) [0x0000000000000000]              
	unsigned char                                      MirrorEnumTest;                                   		// 0x003E (0x0001) [0x0000000000000000]              
	unsigned char                                      InvalidMirrorEnumTest;                            		// 0x003F (0x0001) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30749 ];

		return pClassPointer;
	};

	void CompareDifferentEnumTypes ( );
	void EnumTypeEnumEqualEqualVar ( );
	void EnumEqualEqualVar ( );
};


// Class UnrealScriptTest.Test0013_IfEnumEqualEqualVarDerivedClass
// 0x0000 (0x0040 - 0x0040)
class UTest0013_IfEnumEqualEqualVarDerivedClass : public UTest0013_IfEnumEqualEqualVar
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30767 ];

		return pClassPointer;
	};

	void EnumTypeEnumEqualEqualVar ( );
	void EnumEqualEqualVar ( );
};


// Class UnrealScriptTest.Test0014_FullyQualifiedEnumFunction
// 0x0000 (0x003C - 0x003C)
class UTest0014_FullyQualifiedEnumFunction : public UObject
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30771 ];

		return pClassPointer;
	};

	void Caller ( );
	void FullyQualifiedEnum ( unsigned char val );
};


// Class UnrealScriptTest.Test0014_FullyQualifiedEnumFunctionDerivedClass
// 0x0000 (0x003C - 0x003C)
class UTest0014_FullyQualifiedEnumFunctionDerivedClass : public UTest0014_FullyQualifiedEnumFunction
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30777 ];

		return pClassPointer;
	};

	void Caller ( );
	void FullyQualifiedEnum ( unsigned char val );
};


// Class UnrealScriptTest.Test0015_FullyQualifiedEnumSwitchCase
// 0x0000 (0x003C - 0x003C)
class UTest0015_FullyQualifiedEnumSwitchCase : public UObject
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30782 ];

		return pClassPointer;
	};

	void CaseStatementWithFullQualifiedEnum ( );
};


// Class UnrealScriptTest.Test0015_FullyQualifiedEnumSwitchCaseDerivedClass
// 0x0000 (0x003C - 0x003C)
class UTest0015_FullyQualifiedEnumSwitchCaseDerivedClass : public UTest0015_FullyQualifiedEnumSwitchCase
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30787 ];

		return pClassPointer;
	};

	void CaseStatementWithFullQualifiedEnum ( );
};


// Class UnrealScriptTest.Test0016_PropertyAssignment
// 0x0010 (0x004C - 0x003C)
class UTest0016_PropertyAssignment : public UObject
{
public:
	struct FString                                     TestGlobalString;                                 		// 0x003C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	class UProperty*                                   TestPropertyAssignment;                           		// 0x0048 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30791 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.Test0017_InterfaceAsAVarInAClass
// 0x0000 (0x003C - 0x003C)
class UTest0017_InterfaceAsAVarInAClass : public UObject
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30795 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.Test0018_InterfaceAsAVarInAStruct
// 0x0000 (0x003C - 0x003C)
class UTest0018_InterfaceAsAVarInAStruct : public UObject
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30797 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.Test0019_MacrosAndUnderscores
// 0x0008 (0x0044 - 0x003C)
class UTest0019_MacrosAndUnderscores : public UObject
{
public:
	int                                                TestA;                                            		// 0x003C (0x0004) [0x0000000000000000]              
	int                                                TestB;                                            		// 0x0040 (0x0004) [0x0000000000000000]              

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30801 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.Test0020_Commandlet
// 0x0000 (0x007C - 0x007C)
class UTest0020_Commandlet : public UTestCommandletBase
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30805 ];

		return pClassPointer;
	};

	void PerformStructSerializationTest ( );
	int eventMain ( struct FString Parms );
};


// Class UnrealScriptTest.Test0020_StructDefaults
// 0x005C (0x0098 - 0x003C)
class UTest0020_StructDefaults : public UTestClassBase
{
public:
	struct FTestStruct                                 StructVar_NoDefaults;                             		// 0x003C (0x0020) [0x0000000000000001]              ( CPF_Edit )
	struct FTestStruct                                 StructVar_ClassDefaults;                          		// 0x005C (0x0020) [0x0000000000000001]              ( CPF_Edit )
	TArray< struct FMyCompoundStruct >                 CompoundStructs;                                  		// 0x007C (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	int                                                ClassIntVar;                                      		// 0x0088 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FString                                     ClassStringVar;                                   		// 0x008C (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30811 ];

		return pClassPointer;
	};

	void LogPropertyValues ( struct FTestStruct ParameterStruct );
};


// Class UnrealScriptTest.Test0021_Commandlet
// 0x0000 (0x007C - 0x007C)
class UTest0021_Commandlet : public UTestCommandletBase
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30837 ];

		return pClassPointer;
	};

	int Test0021 ( );
	int eventMain ( struct FString Params );
};


// Class UnrealScriptTest.Test0021_PrivateFunctionLookupBase
// 0x0000 (0x003C - 0x003C)
class UTest0021_PrivateFunctionLookupBase : public UTestClassBase
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30845 ];

		return pClassPointer;
	};

	void DifferentNumberOfParams ( float DeltaTime, struct FRotator CurrentRotation );
	void Test0021 ( );
};


// Class UnrealScriptTest.Test0021_PrivateFunctionLookupDerived
// 0x0000 (0x003C - 0x003C)
class UTest0021_PrivateFunctionLookupDerived : public UTest0021_PrivateFunctionLookupBase
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30844 ];

		return pClassPointer;
	};

	void DifferentNumberOfParams ( float DeltaTime );
};


// Class UnrealScriptTest.Test0022_Commandlet
// 0x0000 (0x007C - 0x007C)
class UTest0022_Commandlet : public UTestCommandletBase
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30858 ];

		return pClassPointer;
	};

	int eventMain ( struct FString Parms );
};


// Class UnrealScriptTest.Test0022_StructMemberAccess
// 0x005C (0x0098 - 0x003C)
class UTest0022_StructMemberAccess : public UTestClassBase
{
public:
	struct FOuterStruct                                m_OuterStruct;                                    		// 0x003C (0x0008) [0x0000000000000000]              
	struct FExtendedOuterStruct                        m_DerivedOuterStruct;                             		// 0x0044 (0x0014) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FCtorStruct                                 m_CtorStruct;                                     		// 0x0058 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	TArray< struct FInnerStruct >                      m_StructArray;                                    		// 0x0064 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FInnerStruct                                m_ConstStruct;                                    		// 0x0070 (0x0004) [0x0000000000000002]              ( CPF_Const )
	struct FScriptDelegate                             m_StructDelegateProp;                             		// 0x0074 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FScriptDelegate                             m_ConstStructDelegateProp;                        		// 0x0080 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FScriptDelegate                             __ReturnStructDelegate__Delegate;                 		// 0x008C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30861 ];

		return pClassPointer;
	};

	void TestInvalidArrayExpansion ( );
	struct FVector ReturnInlineStruct ( );
	struct FOuterStruct ReturnMemberStruct ( );
	void InlineStructAccessTest ( );
	void DelegateReturnValueAccessTest ( );
	void ReturnValueAccessTest ( );
	void ArrayStructAccessTest ( );
	void SimpleStructAccess_CtorTest ( );
	void SimpleStructAccess_NonCtorTest ( );
	struct FOuterStruct ReturnStructDelegate ( );
};


// Class UnrealScriptTest.Test0023_MetaData
// 0x0019 (0x01E5 - 0x01CC)
class ATest0023_MetaData : public AActor
{
public:
	struct FString                                     MemberString;                                     		// 0x01CC (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	struct FExampleStruct                              MemberStruct;                                     		// 0x01D8 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	unsigned char                                      MemberEnum;                                       		// 0x01E4 (0x0001) [0x0000000000000001]              ( CPF_Edit )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30906 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.Test0024_Commandlet
// 0x000C (0x0088 - 0x007C)
class UTest0024_Commandlet : public UTestCommandletBase
{
public:
	struct FScriptDelegate                             __CommandletDelegate__Delegate;                   		// 0x007C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30915 ];

		return pClassPointer;
	};

	void CommandletDelegateFunction ( int Param1 );
	int eventMain ( struct FString Parms );
	void CommandletDelegate ( int I );
};


// Class UnrealScriptTest.Test0024_DelegateComparison
// 0x001C (0x0058 - 0x003C)
class UTest0024_DelegateComparison : public UTestClassBase
{
public:
	class UTest0024_Commandlet*                        CommandletOwner;                                  		// 0x003C (0x0004) [0x0000000000000000]              
	struct FScriptDelegate                             __ComparisonDelegate__Delegate;                   		// 0x0040 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FScriptDelegate                             __SomeOtherDelegate__Delegate;                    		// 0x004C (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30921 ];

		return pClassPointer;
	};

	void SomeOtherDelegate ( unsigned long bFoo );
	void DelegateExample ( class UTest0024_Commandlet* Owner );
	void DelegateFunction ( int Param1 );
	void TestDelegateParam ( struct FScriptDelegate MyDelegate );
	void ComparisonDelegate ( int Param1 );
};


// Class UnrealScriptTest.Test0025_Commandlet
// 0x0004 (0x0080 - 0x007C)
class UTest0025_Commandlet : public UTestCommandletBase
{
public:
	unsigned long                                      FirstBool : 1;                                    		// 0x007C (0x0004) [0x0000000000000000] [0x00000001] 
	unsigned long                                      SecondBool : 1;                                   		// 0x007C (0x0004) [0x0000000000000000] [0x00000002] 
	unsigned long                                      ThirdBool : 1;                                    		// 0x007C (0x0004) [0x0000000000000000] [0x00000004] 

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30941 ];

		return pClassPointer;
	};

	void RunBoolTest ( unsigned long bValueToApply, unsigned long BoolVar );
	int eventMain ( struct FString Params );
};


// Class UnrealScriptTest.TestCommandlet
// 0x0000 (0x007C - 0x007C)
class UTestCommandlet : public UCommandlet
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30960 ];

		return pClassPointer;
	};

	int eventMain ( struct FString Params );
};


// Class UnrealScriptTest.TestPlaceableActor
// 0x0008 (0x01D4 - 0x01CC)
class ATestPlaceableActor : public AActor
{
public:
	class UInheritanceTestDerived*                     ComponentVar;                                     		// 0x01CC (0x0004) [0x00000000040A0009]              ( CPF_Edit | CPF_ExportObject | CPF_EditConst | CPF_Component | CPF_EditInline )
	class UInheritanceTestDerived*                     RuntimeComponent;                                 		// 0x01D0 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30965 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.TestDerivedActor
// 0x0004 (0x01D8 - 0x01D4)
class ATestDerivedActor : public ATestPlaceableActor
{
public:
	class UTest0020_StructDefaults*                    StructDefaultTestObj;                             		// 0x01D4 (0x0004) [0x0000000004400009]              ( CPF_Edit | CPF_ExportObject | CPF_NeedCtorLink | CPF_EditInline )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30971 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.TestPropertyWindowActor
// 0x005C (0x0228 - 0x01CC)
class ATestPropertyWindowActor : public AActor
{
public:
	class UClass*                                      ActorClassVar;                                    		// 0x01CC (0x0004) [0x0000000000000001]              ( CPF_Edit )
	TArray< int >                                      IntArrayVar;                                      		// 0x01D0 (0x000C) [0x0000000004400001]              ( CPF_Edit | CPF_NeedCtorLink | CPF_EditInline )
	struct FNestedArrayStruct                          StructVar;                                        		// 0x01DC (0x0010) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
	TArray< struct FNestedArrayStruct >                StructArrayVar;                                   		// 0x01EC (0x000C) [0x0000000004400001]              ( CPF_Edit | CPF_NeedCtorLink | CPF_EditInline )
	TArray< class UObject* >                           ObjectArray;                                      		// 0x01F8 (0x000C) [0x0000000004400009]              ( CPF_Edit | CPF_ExportObject | CPF_NeedCtorLink | CPF_EditInline )
	class UObject*                                     ObjectStaticArray[ 0x3 ];                         		// 0x0204 (0x000C) [0x0000000004400009]              ( CPF_Edit | CPF_ExportObject | CPF_NeedCtorLink | CPF_EditInline )
	class UInheritanceTestDerived*                     RuntimeComponent;                                 		// 0x0210 (0x0004) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )
	TArray< class UInheritanceTestDerived* >           ComponentDynArray;                                		// 0x0214 (0x000C) [0x0000000004480009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_NeedCtorLink | CPF_EditInline )
	class UInheritanceTestDerived*                     ComponentStaticArray[ 0x2 ];                      		// 0x0220 (0x0008) [0x0000000004080009]              ( CPF_Edit | CPF_ExportObject | CPF_Component | CPF_EditInline )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30975 ];

		return pClassPointer;
	};

};


// Class UnrealScriptTest.UDebuggerRegressionCommandlet
// 0x0000 (0x007C - 0x007C)
class UUDebuggerRegressionCommandlet : public UTestCommandletBase
{
public:

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 30998 ];

		return pClassPointer;
	};

	int eventMain ( struct FString Parms );
};



#ifdef _MSC_VER
	#pragma pack ( pop )
#endif
