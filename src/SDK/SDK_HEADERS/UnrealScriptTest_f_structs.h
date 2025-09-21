/*
#############################################################################################
# Global Agenda (1.5.1.5) SDK
# Generated with TheFeckless UE3 SDK Generator v1.4_Beta-Rev.51
# ========================================================================================= #
# File: UnrealScriptTest_f_structs.h
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
# Function Structs
# ========================================================================================= #
*/

// Function UnrealScriptTest.Test0002_InterfaceNative.TestObjectToInterfaceConversions
// [0x00020000] 
struct UTest0002_InterfaceNative_execTestObjectToInterfaceConversions_Parms
{
};

// Function UnrealScriptTest.Test0002_InterfaceNative.TestInterfaceEvent
// [0x00020800] ( FUNC_Event )
struct UTest0002_InterfaceNative_eventTestInterfaceEvent_Parms
{
	class UObject*                                     ObjParam;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function UnrealScriptTest.Test0002_InterfaceNative.TestNativeFunction
// [0x00020400] ( FUNC_Native )
struct UTest0002_InterfaceNative_execTestNativeFunction_Parms
{
	unsigned long                                      bBoolParm : 1;                                    		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function UnrealScriptTest.Test0010_NativeObject.TestOptionalEventStringParm
// [0x00024802] ( FUNC_Event )
struct UTest0010_NativeObject_eventTestOptionalEventStringParm_Parms
{
	struct FString                                     FirstString;                                      		// 0x0000 (0x000C) [0x0000000000400090]              ( CPF_OptionalParm | CPF_Parm | CPF_NeedCtorLink )
	struct FString                                     SecondString;                                     		// 0x000C (0x000C) [0x0000000000400090]              ( CPF_OptionalParm | CPF_Parm | CPF_NeedCtorLink )
};

// Function UnrealScriptTest.Test0010_NativeObject.FunctionWithMultipleOptionalParms
// [0x00024002] 
struct UTest0010_NativeObject_execFunctionWithMultipleOptionalParms_Parms
{
	struct FString                                     FirstString;                                      		// 0x0000 (0x000C) [0x0000000000400090]              ( CPF_OptionalParm | CPF_Parm | CPF_NeedCtorLink )
	struct FString                                     SecondString;                                     		// 0x000C (0x000C) [0x0000000000400090]              ( CPF_OptionalParm | CPF_Parm | CPF_NeedCtorLink )
};

// Function UnrealScriptTest.Test0010_NativeObject.TestMultipleOptionalParms
// [0x00020802] ( FUNC_Event )
struct UTest0010_NativeObject_eventTestMultipleOptionalParms_Parms
{
};

// Function UnrealScriptTest.Test0010_NativeObject.VerifyConversionFromInterfaceToObjectAsNativeParm
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTest0010_NativeObject_execVerifyConversionFromInterfaceToObjectAsNativeParm_Parms
{
	class UObject*                                     InObject;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	int                                                DummyInt;                                         		// 0x0004 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function UnrealScriptTest.Test0010_NativeObject.TestInterfaceObject_OutParmCompatibility
// [0x00420002] 
struct UTest0010_NativeObject_execTestInterfaceObject_OutParmCompatibility_Parms
{
	class UObject*                                     out_Object;                                       		// 0x0000 (0x0004) [0x0000000000000180]              ( CPF_Parm | CPF_OutParm )
	class UTest0002_InterfaceNative*                   out_Interface;                                    		// 0x0004 (0x0008) [0x0000000000000180]              ( CPF_Parm | CPF_OutParm )
};

// Function UnrealScriptTest.Test0010_NativeObject.Test06_InterfaceToObjectConversions
// [0x00020802] ( FUNC_Event )
struct UTest0010_NativeObject_eventTest06_InterfaceToObjectConversions_Parms
{
	// class UObject*                                  ObjectLocal;                                      		// 0x0000 (0x0004) [0x0000000000000000]              
	// class UTest0002_InterfaceNative*                InterfaceLocal;                                   		// 0x0004 (0x0008) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0010_NativeObject.Test05_StructInheritance
// [0x00020802] ( FUNC_Event )
struct UTest0010_NativeObject_eventTest05_StructInheritance_Parms
{
};

// Function UnrealScriptTest.Test0010_NativeObject.TestObjectToInterfaceConversions
// [0x00020002] 
struct UTest0010_NativeObject_execTestObjectToInterfaceConversions_Parms
{
	// class UTest0002_InterfaceNative*                InterfaceLocal;                                   		// 0x0000 (0x0008) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0010_NativeObject.TestInterfaceEvent
// [0x00020802] ( FUNC_Event )
struct UTest0010_NativeObject_eventTestInterfaceEvent_Parms
{
	class UObject*                                     ObjParam;                                         		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function UnrealScriptTest.Test0010_NativeObject.TestNativeFunction
// [0x00020400] ( FUNC_Native )
struct UTest0010_NativeObject_execTestNativeFunction_Parms
{
	unsigned long                                      bBoolParm : 1;                                    		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function UnrealScriptTest.Test0010_NativeObject.Test03_CallEventWithNativeInterface
// [0x00020802] ( FUNC_Event )
struct UTest0010_NativeObject_eventTest03_CallEventWithNativeInterface_Parms
{
	class UTest0002_InterfaceNative*                   InterfaceParm;                                    		// 0x0000 (0x0008) [0x0000000000000080]              ( CPF_Parm )
};

// Function UnrealScriptTest.Test0010_NativeObject.Test02_PassNativeInterfaceToNativeFunction
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTest0010_NativeObject_execTest02_PassNativeInterfaceToNativeFunction_Parms
{
	class UTest0002_InterfaceNative*                   InterfaceParm;                                    		// 0x0000 (0x0008) [0x0000000000000080]              ( CPF_Parm )
};

// Function UnrealScriptTest.Test0010_NativeObject.Test01_CallEventWithStruct
// [0x00020802] ( FUNC_Event )
struct UTest0010_NativeObject_eventTest01_CallEventWithStruct_Parms
{
	struct FNoCtorProps                                NoCtor;                                           		// 0x0000 (0x000C) [0x0000000000000080]              ( CPF_Parm )
	struct FConstructorStructString                    StringParm;                                       		// 0x000C (0x0018) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FConstructorStructArray                     ArrayParm;                                        		// 0x0024 (0x0010) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	struct FConstructorStructCombo                     ComboParm;                                        		// 0x0034 (0x0020) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	unsigned long                                      PaddingBool : 1;                                  		// 0x0054 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function UnrealScriptTest.Test0010_NativeObject.PerformNativeTest
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTest0010_NativeObject_execPerformNativeTest_Parms
{
	int                                                TestNumber;                                       		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function UnrealScriptTest.Test0011_NativeObjectBoolOrder.NativeTestBoolOrderEvent
// [0x00020800] ( FUNC_Event )
struct UTest0011_NativeObjectBoolOrder_eventNativeTestBoolOrderEvent_Parms
{
	unsigned long                                      EventBoolParm : 1;                                		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function UnrealScriptTest.Test0011_NativeObjectBoolOrder.NativeTestBoolOrderFunction
// [0x00020400] ( FUNC_Native )
struct UTest0011_NativeObjectBoolOrder_execNativeTestBoolOrderFunction_Parms
{
	unsigned long                                      BoolParm : 1;                                     		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	unsigned long                                      ReturnValue : 1;                                  		// 0x0004 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0011_NativeObjectBoolOrder.PerformBoolOrderTest
// [0x00020400] ( FUNC_Native )
struct UTest0011_NativeObjectBoolOrder_execPerformBoolOrderTest_Parms
{
};

// Function UnrealScriptTest.Test0025_NativeIssues.RunTest
// [0x00020002] 
struct UTest0025_NativeIssues_execRunTest_Parms
{
};

// Function UnrealScriptTest.Test0025_NativeIssues.ShowNativeBoolAssignmentTestResults
// [0x00020401] ( FUNC_Final | FUNC_Native )
struct UTest0025_NativeIssues_execShowNativeBoolAssignmentTestResults_Parms
{
};

// Function UnrealScriptTest.Test0003_Commandlet.Test0003
// [0x00020002] 
struct UTest0003_Commandlet_execTest0003_Parms
{
	int                                                ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// int                                             retval;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	// class UTest0003_DerivedClass*                   aClass;                                           		// 0x0008 (0x0004) [0x0000000000000000]              
	// class UTest0003_Interface*                      AnInterface;                                      		// 0x000C (0x0008) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0003_Commandlet.Main
// [0x00020802] ( FUNC_Event )
struct UTest0003_Commandlet_eventMain_Parms
{
	struct FString                                     Params;                                           		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	int                                                ReturnValue;                                      		// 0x000C (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// int                                             retval;                                           		// 0x0010 (0x0004) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0003_Interface.Test0003_FunctionF
// [0x00020000] 
struct UTest0003_Interface_execTest0003_FunctionF_Parms
{
	unsigned char                                      ReturnValue;                                      		// 0x0000 (0x0001) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0003_Interface.Test0003_FunctionE
// [0x00020000] 
struct UTest0003_Interface_execTest0003_FunctionE_Parms
{
	float                                              ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0003_Interface.Test0003_FunctionD
// [0x00020000] 
struct UTest0003_Interface_execTest0003_FunctionD_Parms
{
	int                                                ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0003_Interface.Test0003_FunctionC
// [0x00020000] 
struct UTest0003_Interface_execTest0003_FunctionC_Parms
{
	struct FString                                     ReturnValue;                                      		// 0x0000 (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function UnrealScriptTest.Test0003_Interface.Test0003_FunctionB
// [0x00020000] 
struct UTest0003_Interface_execTest0003_FunctionB_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0003_Interface.Test0003_FunctionA
// [0x00020000] 
struct UTest0003_Interface_execTest0003_FunctionA_Parms
{
};

// Function UnrealScriptTest.Test0003_DerivedClass.Test0003_FunctionF
// [0x00020002] 
struct UTest0003_DerivedClass_execTest0003_FunctionF_Parms
{
	unsigned char                                      ReturnValue;                                      		// 0x0000 (0x0001) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0003_DerivedClass.Test0003_FunctionE
// [0x00020002] 
struct UTest0003_DerivedClass_execTest0003_FunctionE_Parms
{
	float                                              ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0003_DerivedClass.Test0003_FunctionD
// [0x00020002] 
struct UTest0003_DerivedClass_execTest0003_FunctionD_Parms
{
	int                                                ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0003_DerivedClass.Test0003_FunctionC
// [0x00020002] 
struct UTest0003_DerivedClass_execTest0003_FunctionC_Parms
{
	struct FString                                     ReturnValue;                                      		// 0x0000 (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function UnrealScriptTest.Test0003_DerivedClass.Test0003_FunctionB
// [0x00020002] 
struct UTest0003_DerivedClass_execTest0003_FunctionB_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0003_DerivedClass.Test0003_FunctionA
// [0x00020002] 
struct UTest0003_DerivedClass_execTest0003_FunctionA_Parms
{
};

// Function UnrealScriptTest.Test0004_BaseClass.Test0004_FunctionF
// [0x00020002] 
struct UTest0004_BaseClass_execTest0004_FunctionF_Parms
{
	unsigned char                                      ReturnValue;                                      		// 0x0000 (0x0001) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0004_BaseClass.Test0004_FunctionE
// [0x00020002] 
struct UTest0004_BaseClass_execTest0004_FunctionE_Parms
{
	float                                              ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0004_BaseClass.Test0004_FunctionD
// [0x00020002] 
struct UTest0004_BaseClass_execTest0004_FunctionD_Parms
{
	int                                                ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0004_BaseClass.Test0004_FunctionC
// [0x00020002] 
struct UTest0004_BaseClass_execTest0004_FunctionC_Parms
{
	struct FString                                     ReturnValue;                                      		// 0x0000 (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function UnrealScriptTest.Test0004_BaseClass.Test0004_FunctionB
// [0x00020002] 
struct UTest0004_BaseClass_execTest0004_FunctionB_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0004_BaseClass.Test0004_FunctionA
// [0x00020002] 
struct UTest0004_BaseClass_execTest0004_FunctionA_Parms
{
};

// Function UnrealScriptTest.Test0004_Interface.Test0004_FunctionF
// [0x00020000] 
struct UTest0004_Interface_execTest0004_FunctionF_Parms
{
	unsigned char                                      ReturnValue;                                      		// 0x0000 (0x0001) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0004_Interface.Test0004_FunctionE
// [0x00020000] 
struct UTest0004_Interface_execTest0004_FunctionE_Parms
{
	float                                              ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0004_Interface.Test0004_FunctionD
// [0x00020000] 
struct UTest0004_Interface_execTest0004_FunctionD_Parms
{
	int                                                ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0004_Interface.Test0004_FunctionC
// [0x00020000] 
struct UTest0004_Interface_execTest0004_FunctionC_Parms
{
	struct FString                                     ReturnValue;                                      		// 0x0000 (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function UnrealScriptTest.Test0004_Interface.Test0004_FunctionB
// [0x00020000] 
struct UTest0004_Interface_execTest0004_FunctionB_Parms
{
	unsigned long                                      ReturnValue : 1;                                  		// 0x0000 (0x0004) [0x0000000000000580] [0x00000001] ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0004_Interface.Test0004_FunctionA
// [0x00020000] 
struct UTest0004_Interface_execTest0004_FunctionA_Parms
{
};

// Function UnrealScriptTest.Test0004_DerivedClass.Test0004_FunctionA
// [0x00020002] 
struct UTest0004_DerivedClass_execTest0004_FunctionA_Parms
{
};

// Function UnrealScriptTest.Test0005_Commandlet.Test0005
// [0x00020002] 
struct UTest0005_Commandlet_execTest0005_Parms
{
	int                                                ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// int                                             retval;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	// class UTest0005_InterfaceBase*                  ABase;                                            		// 0x0008 (0x0008) [0x0000000000000000]              
	// class UTest0005_InterfaceDerived*               ADerived;                                         		// 0x0010 (0x0008) [0x0000000000000000]              
	// class UTest0005_ImplementingClass*              aClass;                                           		// 0x0018 (0x0004) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0005_Commandlet.Main
// [0x00020802] ( FUNC_Event )
struct UTest0005_Commandlet_eventMain_Parms
{
	struct FString                                     Params;                                           		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	int                                                ReturnValue;                                      		// 0x000C (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// int                                             retval;                                           		// 0x0010 (0x0004) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0005_ImplementingClass.derivedFoo
// [0x00020002] 
struct UTest0005_ImplementingClass_execderivedFoo_Parms
{
	struct FString                                     ReturnValue;                                      		// 0x0000 (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function UnrealScriptTest.Test0005_ImplementingClass.baseFoo
// [0x00020002] 
struct UTest0005_ImplementingClass_execbaseFoo_Parms
{
	struct FString                                     ReturnValue;                                      		// 0x0000 (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function UnrealScriptTest.Test0005_InterfaceDerived.derivedFoo
// [0x00020000] 
struct UTest0005_InterfaceDerived_execderivedFoo_Parms
{
	struct FString                                     ReturnValue;                                      		// 0x0000 (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function UnrealScriptTest.Test0005_InterfaceBase.baseFoo
// [0x00020000] 
struct UTest0005_InterfaceBase_execbaseFoo_Parms
{
	struct FString                                     ReturnValue;                                      		// 0x0000 (0x000C) [0x0000000000400580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )
};

// Function UnrealScriptTest.Test0010_Commandlet.TestInterface
// [0x00020000] 
struct UTest0010_Commandlet_execTestInterface_Parms
{
};

// Function UnrealScriptTest.Test0010_Commandlet.Test0010
// [0x00020002] 
struct UTest0010_Commandlet_execTest0010_Parms
{
	int                                                TestNumber;                                       		// 0x0000 (0x0004) [0x0000000000000880]              ( CPF_Parm | CPF_CoerceParm )
	int                                                ReturnValue;                                      		// 0x0004 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// int                                             retval;                                           		// 0x0008 (0x0004) [0x0000000000000000]              
	// class UTest0010_NativeObject*                   ANativeClass;                                     		// 0x000C (0x0004) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0010_Commandlet.Main
// [0x00020802] ( FUNC_Event )
struct UTest0010_Commandlet_eventMain_Parms
{
	struct FString                                     Params;                                           		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	int                                                ReturnValue;                                      		// 0x000C (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// int                                             retval;                                           		// 0x0010 (0x0004) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0011_Commandlet.PerformBoolOrderTest
// [0x00020003] ( FUNC_Final )
struct UTest0011_Commandlet_execPerformBoolOrderTest_Parms
{
	// class UTest0011_NativeObjectBoolOrder*          TestObj;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0011_Commandlet.Main
// [0x00020802] ( FUNC_Event )
struct UTest0011_Commandlet_eventMain_Parms
{
	struct FString                                     Parms;                                            		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	int                                                ReturnValue;                                      		// 0x000C (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0012_IfVarEqualEqualEnum.VarEqualEqualEnumTypeEnum
// [0x00020002] 
struct UTest0012_IfVarEqualEqualEnum_execVarEqualEqualEnumTypeEnum_Parms
{
	// unsigned char                                   AnEnum;                                           		// 0x0000 (0x0001) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0012_IfVarEqualEqualEnum.VarEqualEqualEnum
// [0x00020002] 
struct UTest0012_IfVarEqualEqualEnum_execVarEqualEqualEnum_Parms
{
	// unsigned char                                   AnEnum;                                           		// 0x0000 (0x0001) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0012_IfVarEqualEqualEnumDerivedClass.VarEqualEqualEnumTypeEnum
// [0x00020000] 
struct UTest0012_IfVarEqualEqualEnumDerivedClass_execVarEqualEqualEnumTypeEnum_Parms
{
};

// Function UnrealScriptTest.Test0012_IfVarEqualEqualEnumDerivedClass.VarEqualEqualEnum
// [0x00020002] 
struct UTest0012_IfVarEqualEqualEnumDerivedClass_execVarEqualEqualEnum_Parms
{
	// unsigned char                                   AnEnum;                                           		// 0x0000 (0x0001) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0013_Commandlet.Main
// [0x00020802] ( FUNC_Event )
struct UTest0013_Commandlet_eventMain_Parms
{
	struct FString                                     Parms;                                            		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	int                                                ReturnValue;                                      		// 0x000C (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class UTest0013_IfEnumEqualEqualVar*            TestObj;                                          		// 0x0010 (0x0004) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0013_IfEnumEqualEqualVar.CompareDifferentEnumTypes
// [0x00020002] 
struct UTest0013_IfEnumEqualEqualVar_execCompareDifferentEnumTypes_Parms
{
	// class AActor*                                   TestActor;                                        		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0013_IfEnumEqualEqualVar.EnumTypeEnumEqualEqualVar
// [0x00020002] 
struct UTest0013_IfEnumEqualEqualVar_execEnumTypeEnumEqualEqualVar_Parms
{
	// unsigned char                                   AnEnum;                                           		// 0x0000 (0x0001) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0013_IfEnumEqualEqualVar.EnumEqualEqualVar
// [0x00020000] 
struct UTest0013_IfEnumEqualEqualVar_execEnumEqualEqualVar_Parms
{
};

// Function UnrealScriptTest.Test0013_IfEnumEqualEqualVarDerivedClass.EnumTypeEnumEqualEqualVar
// [0x00020000] 
struct UTest0013_IfEnumEqualEqualVarDerivedClass_execEnumTypeEnumEqualEqualVar_Parms
{
};

// Function UnrealScriptTest.Test0013_IfEnumEqualEqualVarDerivedClass.EnumEqualEqualVar
// [0x00020000] 
struct UTest0013_IfEnumEqualEqualVarDerivedClass_execEnumEqualEqualVar_Parms
{
};

// Function UnrealScriptTest.Test0014_FullyQualifiedEnumFunction.Caller
// [0x00020002] 
struct UTest0014_FullyQualifiedEnumFunction_execCaller_Parms
{
};

// Function UnrealScriptTest.Test0014_FullyQualifiedEnumFunction.FullyQualifiedEnum
// [0x00020000] 
struct UTest0014_FullyQualifiedEnumFunction_execFullyQualifiedEnum_Parms
{
	unsigned char                                      val;                                              		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
};

// Function UnrealScriptTest.Test0014_FullyQualifiedEnumFunctionDerivedClass.Caller
// [0x00020002] 
struct UTest0014_FullyQualifiedEnumFunctionDerivedClass_execCaller_Parms
{
};

// Function UnrealScriptTest.Test0014_FullyQualifiedEnumFunctionDerivedClass.FullyQualifiedEnum
// [0x00020000] 
struct UTest0014_FullyQualifiedEnumFunctionDerivedClass_execFullyQualifiedEnum_Parms
{
	unsigned char                                      val;                                              		// 0x0000 (0x0001) [0x0000000000000080]              ( CPF_Parm )
};

// Function UnrealScriptTest.Test0015_FullyQualifiedEnumSwitchCase.CaseStatementWithFullQualifiedEnum
// [0x00020002] 
struct UTest0015_FullyQualifiedEnumSwitchCase_execCaseStatementWithFullQualifiedEnum_Parms
{
	// unsigned char                                   AnEnum;                                           		// 0x0000 (0x0001) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0015_FullyQualifiedEnumSwitchCaseDerivedClass.CaseStatementWithFullQualifiedEnum
// [0x00020002] 
struct UTest0015_FullyQualifiedEnumSwitchCaseDerivedClass_execCaseStatementWithFullQualifiedEnum_Parms
{
	// unsigned char                                   AnEnum;                                           		// 0x0000 (0x0001) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0020_Commandlet.PerformStructSerializationTest
// [0x00020003] ( FUNC_Final )
struct UTest0020_Commandlet_execPerformStructSerializationTest_Parms
{
	// class UTest0020_StructDefaults*                 TestObj;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0020_Commandlet.Main
// [0x00020802] ( FUNC_Event )
struct UTest0020_Commandlet_eventMain_Parms
{
	struct FString                                     Parms;                                            		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	int                                                ReturnValue;                                      		// 0x000C (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0020_StructDefaults.LogPropertyValues
// [0x00024001] ( FUNC_Final )
struct UTest0020_StructDefaults_execLogPropertyValues_Parms
{
	struct FTestStruct                                 ParameterStruct;                                  		// 0x0000 (0x0020) [0x0000000000000090]              ( CPF_OptionalParm | CPF_Parm )
};

// Function UnrealScriptTest.Test0021_Commandlet.Test0021
// [0x00020002] 
struct UTest0021_Commandlet_execTest0021_Parms
{
	int                                                ReturnValue;                                      		// 0x0000 (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// int                                             Result;                                           		// 0x0004 (0x0004) [0x0000000000000000]              
	// class UTest0021_PrivateFunctionLookupDerived*   TestObject;                                       		// 0x0008 (0x0004) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0021_Commandlet.Main
// [0x00020802] ( FUNC_Event )
struct UTest0021_Commandlet_eventMain_Parms
{
	struct FString                                     Params;                                           		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	int                                                ReturnValue;                                      		// 0x000C (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// int                                             retval;                                           		// 0x0010 (0x0004) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0021_PrivateFunctionLookupDerived.DifferentNumberOfParams
// [0x00020002] 
struct UTest0021_PrivateFunctionLookupDerived_execDifferentNumberOfParams_Parms
{
	float                                              DeltaTime;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function UnrealScriptTest.Test0021_PrivateFunctionLookupBase.DifferentNumberOfParams
// [0x00040003] ( FUNC_Final )
struct UTest0021_PrivateFunctionLookupBase_execDifferentNumberOfParams_Parms
{
	float                                              DeltaTime;                                        		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
	struct FRotator                                    CurrentRotation;                                  		// 0x0004 (0x000C) [0x0000000000000080]              ( CPF_Parm )
};

// Function UnrealScriptTest.Test0021_PrivateFunctionLookupBase.Test0021
// [0x00820002] 
struct UTest0021_PrivateFunctionLookupBase_execTest0021_Parms
{
	// struct FRotator                                 Rotation;                                         		// 0x0000 (0x000C) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0022_Commandlet.Main
// [0x00020802] ( FUNC_Event )
struct UTest0022_Commandlet_eventMain_Parms
{
	struct FString                                     Parms;                                            		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	int                                                ReturnValue;                                      		// 0x000C (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// int                                             TestId;                                           		// 0x0010 (0x0004) [0x0000000000000000]              
	// class UTest0022_StructMemberAccess*             TestObject;                                       		// 0x0014 (0x0004) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.TestInvalidArrayExpansion
// [0x00020002] 
struct UTest0022_StructMemberAccess_execTestInvalidArrayExpansion_Parms
{
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.ReturnInlineStruct
// [0x00040003] ( FUNC_Final )
struct UTest0022_StructMemberAccess_execReturnInlineStruct_Parms
{
	struct FVector                                     ReturnValue;                                      		// 0x0000 (0x000C) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.ReturnMemberStruct
// [0x00040003] ( FUNC_Final )
struct UTest0022_StructMemberAccess_execReturnMemberStruct_Parms
{
	struct FOuterStruct                                ReturnValue;                                      		// 0x0000 (0x0008) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.InlineStructAccessTest
// [0x00020002] 
struct UTest0022_StructMemberAccess_execInlineStructAccessTest_Parms
{
	// float                                           VectorX;                                          		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.DelegateReturnValueAccessTest
// [0x00020002] 
struct UTest0022_StructMemberAccess_execDelegateReturnValueAccessTest_Parms
{
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.ReturnValueAccessTest
// [0x00820002] 
struct UTest0022_StructMemberAccess_execReturnValueAccessTest_Parms
{
	// struct FOuterStruct                             LocalStruct;                                      		// 0x0000 (0x0008) [0x0000000000000000]              
	// float                                           LocalFloat;                                       		// 0x0008 (0x0004) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.ArrayStructAccessTest
// [0x00020002] 
struct UTest0022_StructMemberAccess_execArrayStructAccessTest_Parms
{
	// int                                             ArrayInt;                                         		// 0x0000 (0x0004) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.SimpleStructAccess_CtorTest
// [0x00020002] 
struct UTest0022_StructMemberAccess_execSimpleStructAccess_CtorTest_Parms
{
	// struct FString                                  ValueString;                                      		// 0x0000 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.SimpleStructAccess_NonCtorTest
// [0x00020002] 
struct UTest0022_StructMemberAccess_execSimpleStructAccess_NonCtorTest_Parms
{
	// float                                           ValueFloat;                                       		// 0x0000 (0x0004) [0x0000000000000000]              
	// int                                             ValueInt;                                         		// 0x0004 (0x0004) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.ReturnStructDelegate
// [0x00120000] 
struct UTest0022_StructMemberAccess_execReturnStructDelegate_Parms
{
	struct FOuterStruct                                ReturnValue;                                      		// 0x0000 (0x0008) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.Test0024_Commandlet.CommandletDelegateFunction
// [0x00020002] 
struct UTest0024_Commandlet_execCommandletDelegateFunction_Parms
{
	int                                                Param1;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function UnrealScriptTest.Test0024_Commandlet.Main
// [0x00020802] ( FUNC_Event )
struct UTest0024_Commandlet_eventMain_Parms
{
	struct FString                                     Parms;                                            		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	int                                                ReturnValue;                                      		// 0x000C (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// class UTest0024_DelegateComparison*             TestObj;                                          		// 0x0010 (0x0004) [0x0000000000000000]              
};

// Function UnrealScriptTest.Test0024_Commandlet.CommandletDelegate
// [0x00120002] 
struct UTest0024_Commandlet_execCommandletDelegate_Parms
{
	int                                                I;                                                		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function UnrealScriptTest.Test0024_DelegateComparison.SomeOtherDelegate
// [0x00120000] 
struct UTest0024_DelegateComparison_execSomeOtherDelegate_Parms
{
	unsigned long                                      bFoo : 1;                                         		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function UnrealScriptTest.Test0024_DelegateComparison.DelegateExample
// [0x00020002] 
struct UTest0024_DelegateComparison_execDelegateExample_Parms
{
	class UTest0024_Commandlet*                        Owner;                                            		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function UnrealScriptTest.Test0024_DelegateComparison.DelegateFunction
// [0x00020002] 
struct UTest0024_DelegateComparison_execDelegateFunction_Parms
{
	int                                                Param1;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function UnrealScriptTest.Test0024_DelegateComparison.TestDelegateParam
// [0x00020002] 
struct UTest0024_DelegateComparison_execTestDelegateParam_Parms
{
	struct FScriptDelegate                             MyDelegate;                                       		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
};

// Function UnrealScriptTest.Test0024_DelegateComparison.ComparisonDelegate
// [0x00120002] 
struct UTest0024_DelegateComparison_execComparisonDelegate_Parms
{
	int                                                Param1;                                           		// 0x0000 (0x0004) [0x0000000000000080]              ( CPF_Parm )
};

// Function UnrealScriptTest.Test0025_Commandlet.RunBoolTest
// [0x00020002] 
struct UTest0025_Commandlet_execRunBoolTest_Parms
{
	unsigned long                                      bValueToApply : 1;                                		// 0x0000 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
	unsigned long                                      BoolVar : 1;                                      		// 0x0004 (0x0004) [0x0000000000000080] [0x00000001] ( CPF_Parm )
};

// Function UnrealScriptTest.Test0025_Commandlet.Main
// [0x00020802] ( FUNC_Event )
struct UTest0025_Commandlet_eventMain_Parms
{
	struct FString                                     Params;                                           		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	int                                                ReturnValue;                                      		// 0x000C (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// unsigned long                                   bFirstLocalBool : 1;                              		// 0x0010 (0x0004) [0x0000000000000000] [0x00000001] 
	// unsigned long                                   bSecondLocalBool : 1;                             		// 0x0014 (0x0004) [0x0000000000000000] [0x00000001] 
	// unsigned long                                   bThirdLocalBool : 1;                              		// 0x0018 (0x0004) [0x0000000000000000] [0x00000001] 
};

// Function UnrealScriptTest.TestCommandlet.Main
// [0x00020802] ( FUNC_Event )
struct UTestCommandlet_eventMain_Parms
{
	struct FString                                     Params;                                           		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	int                                                ReturnValue;                                      		// 0x000C (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
};

// Function UnrealScriptTest.UDebuggerRegressionCommandlet.Main
// [0x00020802] ( FUNC_Event )
struct UUDebuggerRegressionCommandlet_eventMain_Parms
{
	struct FString                                     Parms;                                            		// 0x0000 (0x000C) [0x0000000000400080]              ( CPF_Parm | CPF_NeedCtorLink )
	int                                                ReturnValue;                                      		// 0x000C (0x0004) [0x0000000000000580]              ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
	// TArray< class AActor* >                         T;                                                		// 0x0010 (0x000C) [0x0000000000400000]              ( CPF_NeedCtorLink )
	// int                                             I;                                                		// 0x001C (0x0004) [0x0000000000000000]              
};


#ifdef _MSC_VER
	#pragma pack ( pop )
#endif