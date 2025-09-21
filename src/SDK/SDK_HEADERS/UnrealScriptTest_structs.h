/*
#############################################################################################
# Global Agenda (1.5.1.5) SDK
# Generated with TheFeckless UE3 SDK Generator v1.4_Beta-Rev.51
# ========================================================================================= #
# File: UnrealScriptTest_structs.h
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
# Script Structs
# ========================================================================================= #
*/

// ScriptStruct UnrealScriptTest.CTComplexNestRoot.ComplexNestStruct
// 0x0010
struct FComplexNestStruct
{
	class UCTSimpleNestObject*                         ComplexNestStructObject;                          		// 0x0000 (0x0004) [0x0000000004400009]              ( CPF_Edit | CPF_ExportObject | CPF_NeedCtorLink | CPF_EditInline )
	TArray< class UCTSimpleNestObject* >               ComplexNestStructObjectArray;                     		// 0x0004 (0x000C) [0x0000000004400009]              ( CPF_Edit | CPF_ExportObject | CPF_NeedCtorLink | CPF_EditInline )
};

// ScriptStruct UnrealScriptTest.NestingTest_FirstLevelSubobject.NestedObjectStruct
// 0x0010
struct FNestedObjectStruct
{
	class UNestingTest_SecondLevelSubobject*           StructObject;                                     		// 0x0000 (0x0004) [0x0000000004400009]              ( CPF_Edit | CPF_ExportObject | CPF_NeedCtorLink | CPF_EditInline )
	TArray< class UNestingTest_SecondLevelSubobject* > StructObjectArray;                                		// 0x0004 (0x000C) [0x0000000004400009]              ( CPF_Edit | CPF_ExportObject | CPF_NeedCtorLink | CPF_EditInline )
};

// ScriptStruct UnrealScriptTest.Test0010_NativeObject.MyFirstStruct
// 0x0014
struct FMyFirstStruct
{
	int                                                MyFirstInt;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	float                                              MyFirstFloat;                                     		// 0x0004 (0x0004) [0x0000000000000000]              
	struct FString                                     MyFirstString;                                    		// 0x0008 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct UnrealScriptTest.Test0010_NativeObject.MyStruct
// 0x0044(0x0058 - 0x0014)
struct UTest0010_NativeObject_FMyStruct : FMyFirstStruct
{
	int                                                MyInt;                                            		// 0x0014 (0x0004) [0x0000000000000000]              
	float                                              MyFloat;                                          		// 0x0018 (0x0004) [0x0000000000000000]              
	struct FString                                     MyStrings[ 0x5 ];                                 		// 0x001C (0x003C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct UnrealScriptTest.Test0010_NativeObject.ConstructorStructCombo
// 0x0020
struct FConstructorStructCombo
{
	float                                              FloatNoCtor;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	TArray< struct FString >                           StringArrayNeedCtor;                              		// 0x0004 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	struct FString                                     StringNeedCtor;                                   		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                IntNoCtor;                                        		// 0x001C (0x0004) [0x0000000000000000]              
};

// ScriptStruct UnrealScriptTest.Test0010_NativeObject.ConstructorStructArray
// 0x0010
struct FConstructorStructArray
{
	int                                                IntNoCtor;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
	TArray< struct FString >                           StringArrayNeedCtor;                              		// 0x0004 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct UnrealScriptTest.Test0010_NativeObject.ConstructorStructString
// 0x0018
struct FConstructorStructString
{
	struct FString                                     StringNeedCtor;                                   		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	int                                                IntNoCtor;                                        		// 0x000C (0x0004) [0x0000000000000000]              
	class UTest0002_InterfaceNative*                   InterfaceVar;                                     		// 0x0010 (0x0008) [0x0000000000000000]              
	unsigned char                                      UnknownData00[ 0x4 ];                             		// 0x0014 (0x0004) FIX WRONG TYPE SIZE OF PREVIUS PROPERTY
};

// ScriptStruct UnrealScriptTest.Test0010_NativeObject.NoCtorProps
// 0x000C
struct FNoCtorProps
{
	int                                                Foo1;                                             		// 0x0000 (0x0004) [0x0000000000000000]              
	unsigned long                                      Foo2 : 1;                                         		// 0x0004 (0x0004) [0x0000000000000000] [0x00000001] 
	float                                              Foo3;                                             		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct UnrealScriptTest.Test0018_InterfaceAsAVarInAStruct.MyStruct
// 0x0004
struct UTest0018_InterfaceAsAVarInAStruct_FMyStruct
{
	int                                                tmp;                                              		// 0x0000 (0x0004) [0x0000000000000000]              
};

// ScriptStruct UnrealScriptTest.Test0020_StructDefaults.MyAtomicStruct
// 0x0004
struct FMyAtomicStruct
{
	int                                                foo;                                              		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct UnrealScriptTest.Test0020_StructDefaults.MyCompoundStruct
// 0x0004
struct FMyCompoundStruct
{
	struct FMyAtomicStruct                             atomicStruct;                                     		// 0x0000 (0x0004) [0x0000000000000000]              
};

// ScriptStruct UnrealScriptTest.Test0020_StructDefaults.TestInnerNestedStruct
// 0x0004
struct FTestInnerNestedStruct
{
	int                                                InnerNestedStructInt;                             		// 0x0000 (0x0004) [0x0000000000000000]              
};

// ScriptStruct UnrealScriptTest.Test0020_StructDefaults.TestNestedStruct
// 0x000C
struct FTestNestedStruct
{
	int                                                NestedStructInt;                                  		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FTestInnerNestedStruct                      InnerStructNoOverride;                            		// 0x0004 (0x0004) [0x0000000000000000]              
	struct FTestInnerNestedStruct                      InnerStructOverrideValue;                         		// 0x0008 (0x0004) [0x0000000000000000]              
};

// ScriptStruct UnrealScriptTest.Test0020_StructDefaults.TestStruct
// 0x0020
struct FTestStruct
{
	int                                                TestInt;                                          		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	float                                              TestFloat;                                        		// 0x0004 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	struct FTestNestedStruct                           TestMemberStruct;                                 		// 0x0008 (0x000C) [0x0000000000000001]              ( CPF_Edit )
	struct FTestNestedStruct                           TestMemberStruct2;                                		// 0x0014 (0x000C) [0x0000000000000001]              ( CPF_Edit )
};

// ScriptStruct UnrealScriptTest.Test0022_StructMemberAccess.InnerStruct
// 0x0004
struct FInnerStruct
{
	int                                                InnerStructInt;                                   		// 0x0000 (0x0004) [0x0000000000000000]              
};

// ScriptStruct UnrealScriptTest.Test0022_StructMemberAccess.OuterStruct
// 0x0008
struct FOuterStruct
{
	float                                              OuterStruct_Float;                                		// 0x0000 (0x0004) [0x0000000000000000]              
	struct FInnerStruct                                OuterStruct_NestedStruct;                         		// 0x0004 (0x0004) [0x0000000000000000]              
};

// ScriptStruct UnrealScriptTest.Test0022_StructMemberAccess.ExtendedOuterStruct
// 0x000C(0x0014 - 0x0008)
struct FExtendedOuterStruct : FOuterStruct
{
	TArray< struct FInnerStruct >                      OuterStruct_StructArray;                          		// 0x0008 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct UnrealScriptTest.Test0022_StructMemberAccess.CtorStruct
// 0x000C
struct FCtorStruct
{
	struct FString                                     CtorStruct_String;                                		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// ScriptStruct UnrealScriptTest.Test0023_MetaData.ExampleStruct
// 0x000C
struct FExampleStruct
{
	struct FString                                     ExampleStructString;                              		// 0x0000 (0x000C) [0x0000000000400001]              ( CPF_Edit | CPF_NeedCtorLink )
};

// ScriptStruct UnrealScriptTest.TestPropertyWindowActor.NestedArrayStruct
// 0x0010
struct FNestedArrayStruct
{
	int                                                NormalIntVar;                                     		// 0x0000 (0x0004) [0x0000000000000001]              ( CPF_Edit )
	TArray< struct FString >                           NestedStringArray;                                		// 0x0004 (0x000C) [0x0000000004400001]              ( CPF_Edit | CPF_NeedCtorLink | CPF_EditInline )
};


#ifdef _MSC_VER
	#pragma pack ( pop )
#endif