#pragma once
#include "src/pch.hpp"

/*
#############################################################################################
# Global Agenda (1.5.1.5) SDK
# Generated with TheFeckless UE3 SDK Generator v1.4_Beta-Rev.51
# ========================================================================================= #
# File: UnrealScriptTest_functions.h
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
# Functions
# ========================================================================================= #
*/

// Function UnrealScriptTest.Test0002_InterfaceNative.TestObjectToInterfaceConversions
// [0x00020000] 
// Parameters infos:

void UTest0002_InterfaceNative::TestObjectToInterfaceConversions ( )
{
	static UFunction* pFnTestObjectToInterfaceConversions = NULL;

	if ( ! pFnTestObjectToInterfaceConversions )
		pFnTestObjectToInterfaceConversions = (UFunction*) UObject::GObjObjects()->Data[ 30531 ];

	UTest0002_InterfaceNative_execTestObjectToInterfaceConversions_Parms TestObjectToInterfaceConversions_Parms;

	this->ProcessEvent ( pFnTestObjectToInterfaceConversions, &TestObjectToInterfaceConversions_Parms, NULL );
};

// Function UnrealScriptTest.Test0002_InterfaceNative.TestInterfaceEvent
// [0x00020800] ( FUNC_Event )
// Parameters infos:
// class UObject*                 ObjParam                       ( CPF_Parm )

void UTest0002_InterfaceNative::eventTestInterfaceEvent ( class UObject* ObjParam )
{
	static UFunction* pFnTestInterfaceEvent = NULL;

	if ( ! pFnTestInterfaceEvent )
		pFnTestInterfaceEvent = (UFunction*) UObject::GObjObjects()->Data[ 30529 ];

	UTest0002_InterfaceNative_eventTestInterfaceEvent_Parms TestInterfaceEvent_Parms;
	TestInterfaceEvent_Parms.ObjParam = ObjParam;

	this->ProcessEvent ( pFnTestInterfaceEvent, &TestInterfaceEvent_Parms, NULL );
};

// Function UnrealScriptTest.Test0002_InterfaceNative.TestNativeFunction
// [0x00020400] ( FUNC_Native )
// Parameters infos:
// unsigned long                  bBoolParm                      ( CPF_Parm )

void UTest0002_InterfaceNative::TestNativeFunction ( unsigned long bBoolParm )
{
	static UFunction* pFnTestNativeFunction = NULL;

	if ( ! pFnTestNativeFunction )
		pFnTestNativeFunction = (UFunction*) UObject::GObjObjects()->Data[ 30527 ];

	UTest0002_InterfaceNative_execTestNativeFunction_Parms TestNativeFunction_Parms;
	TestNativeFunction_Parms.bBoolParm = bBoolParm;

	pFnTestNativeFunction->FunctionFlags |= ~0x400;

	this->ProcessEvent ( pFnTestNativeFunction, &TestNativeFunction_Parms, NULL );

	pFnTestNativeFunction->FunctionFlags |= 0x400;
};

// Function UnrealScriptTest.Test0010_NativeObject.TestOptionalEventStringParm
// [0x00024802] ( FUNC_Event )
// Parameters infos:
// struct FString                 FirstString                    ( CPF_OptionalParm | CPF_Parm | CPF_NeedCtorLink )
// struct FString                 SecondString                   ( CPF_OptionalParm | CPF_Parm | CPF_NeedCtorLink )

void UTest0010_NativeObject::eventTestOptionalEventStringParm ( struct FString FirstString, struct FString SecondString )
{
	static UFunction* pFnTestOptionalEventStringParm = NULL;

	if ( ! pFnTestOptionalEventStringParm )
		pFnTestOptionalEventStringParm = (UFunction*) UObject::GObjObjects()->Data[ 30715 ];

	UTest0010_NativeObject_eventTestOptionalEventStringParm_Parms TestOptionalEventStringParm_Parms;
	memcpy ( &TestOptionalEventStringParm_Parms.FirstString, &FirstString, 0xC );
	memcpy ( &TestOptionalEventStringParm_Parms.SecondString, &SecondString, 0xC );

	this->ProcessEvent ( pFnTestOptionalEventStringParm, &TestOptionalEventStringParm_Parms, NULL );
};

// Function UnrealScriptTest.Test0010_NativeObject.FunctionWithMultipleOptionalParms
// [0x00024002] 
// Parameters infos:
// struct FString                 FirstString                    ( CPF_OptionalParm | CPF_Parm | CPF_NeedCtorLink )
// struct FString                 SecondString                   ( CPF_OptionalParm | CPF_Parm | CPF_NeedCtorLink )

void UTest0010_NativeObject::FunctionWithMultipleOptionalParms ( struct FString FirstString, struct FString SecondString )
{
	static UFunction* pFnFunctionWithMultipleOptionalParms = NULL;

	if ( ! pFnFunctionWithMultipleOptionalParms )
		pFnFunctionWithMultipleOptionalParms = (UFunction*) UObject::GObjObjects()->Data[ 30712 ];

	UTest0010_NativeObject_execFunctionWithMultipleOptionalParms_Parms FunctionWithMultipleOptionalParms_Parms;
	memcpy ( &FunctionWithMultipleOptionalParms_Parms.FirstString, &FirstString, 0xC );
	memcpy ( &FunctionWithMultipleOptionalParms_Parms.SecondString, &SecondString, 0xC );

	this->ProcessEvent ( pFnFunctionWithMultipleOptionalParms, &FunctionWithMultipleOptionalParms_Parms, NULL );
};

// Function UnrealScriptTest.Test0010_NativeObject.TestMultipleOptionalParms
// [0x00020802] ( FUNC_Event )
// Parameters infos:

void UTest0010_NativeObject::eventTestMultipleOptionalParms ( )
{
	static UFunction* pFnTestMultipleOptionalParms = NULL;

	if ( ! pFnTestMultipleOptionalParms )
		pFnTestMultipleOptionalParms = (UFunction*) UObject::GObjObjects()->Data[ 30711 ];

	UTest0010_NativeObject_eventTestMultipleOptionalParms_Parms TestMultipleOptionalParms_Parms;

	this->ProcessEvent ( pFnTestMultipleOptionalParms, &TestMultipleOptionalParms_Parms, NULL );
};

// Function UnrealScriptTest.Test0010_NativeObject.VerifyConversionFromInterfaceToObjectAsNativeParm
// [0x00020401] ( FUNC_Final | FUNC_Native )
// Parameters infos:
// class UObject*                 InObject                       ( CPF_Parm )
// int                            DummyInt                       ( CPF_Parm )

void UTest0010_NativeObject::VerifyConversionFromInterfaceToObjectAsNativeParm ( class UObject* InObject, int DummyInt )
{
	static UFunction* pFnVerifyConversionFromInterfaceToObjectAsNativeParm = NULL;

	if ( ! pFnVerifyConversionFromInterfaceToObjectAsNativeParm )
		pFnVerifyConversionFromInterfaceToObjectAsNativeParm = (UFunction*) UObject::GObjObjects()->Data[ 30705 ];

	UTest0010_NativeObject_execVerifyConversionFromInterfaceToObjectAsNativeParm_Parms VerifyConversionFromInterfaceToObjectAsNativeParm_Parms;
	VerifyConversionFromInterfaceToObjectAsNativeParm_Parms.InObject = InObject;
	VerifyConversionFromInterfaceToObjectAsNativeParm_Parms.DummyInt = DummyInt;

	pFnVerifyConversionFromInterfaceToObjectAsNativeParm->FunctionFlags |= ~0x400;

	this->ProcessEvent ( pFnVerifyConversionFromInterfaceToObjectAsNativeParm, &VerifyConversionFromInterfaceToObjectAsNativeParm_Parms, NULL );

	pFnVerifyConversionFromInterfaceToObjectAsNativeParm->FunctionFlags |= 0x400;
};

// Function UnrealScriptTest.Test0010_NativeObject.TestInterfaceObject_OutParmCompatibility
// [0x00420002] 
// Parameters infos:
// class UObject*                 out_Object                     ( CPF_Parm | CPF_OutParm )
// class UTest0002_InterfaceNative* out_Interface                  ( CPF_Parm | CPF_OutParm )

void UTest0010_NativeObject::TestInterfaceObject_OutParmCompatibility ( class UObject** out_Object, class UTest0002_InterfaceNative** out_Interface )
{
	static UFunction* pFnTestInterfaceObject_OutParmCompatibility = NULL;

	if ( ! pFnTestInterfaceObject_OutParmCompatibility )
		pFnTestInterfaceObject_OutParmCompatibility = (UFunction*) UObject::GObjObjects()->Data[ 30706 ];

	UTest0010_NativeObject_execTestInterfaceObject_OutParmCompatibility_Parms TestInterfaceObject_OutParmCompatibility_Parms;

	this->ProcessEvent ( pFnTestInterfaceObject_OutParmCompatibility, &TestInterfaceObject_OutParmCompatibility_Parms, NULL );

	if ( out_Object )
		*out_Object = TestInterfaceObject_OutParmCompatibility_Parms.out_Object;

	if ( out_Interface )
		*out_Interface = TestInterfaceObject_OutParmCompatibility_Parms.out_Interface;
};

// Function UnrealScriptTest.Test0010_NativeObject.Test06_InterfaceToObjectConversions
// [0x00020802] ( FUNC_Event )
// Parameters infos:

void UTest0010_NativeObject::eventTest06_InterfaceToObjectConversions ( )
{
	static UFunction* pFnTest06_InterfaceToObjectConversions = NULL;

	if ( ! pFnTest06_InterfaceToObjectConversions )
		pFnTest06_InterfaceToObjectConversions = (UFunction*) UObject::GObjObjects()->Data[ 30702 ];

	UTest0010_NativeObject_eventTest06_InterfaceToObjectConversions_Parms Test06_InterfaceToObjectConversions_Parms;

	this->ProcessEvent ( pFnTest06_InterfaceToObjectConversions, &Test06_InterfaceToObjectConversions_Parms, NULL );
};

// Function UnrealScriptTest.Test0010_NativeObject.Test05_StructInheritance
// [0x00020802] ( FUNC_Event )
// Parameters infos:

void UTest0010_NativeObject::eventTest05_StructInheritance ( )
{
	static UFunction* pFnTest05_StructInheritance = NULL;

	if ( ! pFnTest05_StructInheritance )
		pFnTest05_StructInheritance = (UFunction*) UObject::GObjObjects()->Data[ 30701 ];

	UTest0010_NativeObject_eventTest05_StructInheritance_Parms Test05_StructInheritance_Parms;

	this->ProcessEvent ( pFnTest05_StructInheritance, &Test05_StructInheritance_Parms, NULL );
};

// Function UnrealScriptTest.Test0010_NativeObject.TestObjectToInterfaceConversions
// [0x00020002] 
// Parameters infos:

void UTest0010_NativeObject::TestObjectToInterfaceConversions ( )
{
	static UFunction* pFnTestObjectToInterfaceConversions = NULL;

	if ( ! pFnTestObjectToInterfaceConversions )
		pFnTestObjectToInterfaceConversions = (UFunction*) UObject::GObjObjects()->Data[ 30699 ];

	UTest0010_NativeObject_execTestObjectToInterfaceConversions_Parms TestObjectToInterfaceConversions_Parms;

	this->ProcessEvent ( pFnTestObjectToInterfaceConversions, &TestObjectToInterfaceConversions_Parms, NULL );
};

// Function UnrealScriptTest.Test0010_NativeObject.TestInterfaceEvent
// [0x00020802] ( FUNC_Event )
// Parameters infos:
// class UObject*                 ObjParam                       ( CPF_Parm )

void UTest0010_NativeObject::eventTestInterfaceEvent ( class UObject* ObjParam )
{
	static UFunction* pFnTestInterfaceEvent = NULL;

	if ( ! pFnTestInterfaceEvent )
		pFnTestInterfaceEvent = (UFunction*) UObject::GObjObjects()->Data[ 30697 ];

	UTest0010_NativeObject_eventTestInterfaceEvent_Parms TestInterfaceEvent_Parms;
	TestInterfaceEvent_Parms.ObjParam = ObjParam;

	this->ProcessEvent ( pFnTestInterfaceEvent, &TestInterfaceEvent_Parms, NULL );
};

// Function UnrealScriptTest.Test0010_NativeObject.TestNativeFunction
// [0x00020400] ( FUNC_Native )
// Parameters infos:
// unsigned long                  bBoolParm                      ( CPF_Parm )

void UTest0010_NativeObject::TestNativeFunction ( unsigned long bBoolParm )
{
	static UFunction* pFnTestNativeFunction = NULL;

	if ( ! pFnTestNativeFunction )
		pFnTestNativeFunction = (UFunction*) UObject::GObjObjects()->Data[ 30695 ];

	UTest0010_NativeObject_execTestNativeFunction_Parms TestNativeFunction_Parms;
	TestNativeFunction_Parms.bBoolParm = bBoolParm;

	pFnTestNativeFunction->FunctionFlags |= ~0x400;

	this->ProcessEvent ( pFnTestNativeFunction, &TestNativeFunction_Parms, NULL );

	pFnTestNativeFunction->FunctionFlags |= 0x400;
};

// Function UnrealScriptTest.Test0010_NativeObject.Test03_CallEventWithNativeInterface
// [0x00020802] ( FUNC_Event )
// Parameters infos:
// class UTest0002_InterfaceNative* InterfaceParm                  ( CPF_Parm )

void UTest0010_NativeObject::eventTest03_CallEventWithNativeInterface ( class UTest0002_InterfaceNative* InterfaceParm )
{
	static UFunction* pFnTest03_CallEventWithNativeInterface = NULL;

	if ( ! pFnTest03_CallEventWithNativeInterface )
		pFnTest03_CallEventWithNativeInterface = (UFunction*) UObject::GObjObjects()->Data[ 30693 ];

	UTest0010_NativeObject_eventTest03_CallEventWithNativeInterface_Parms Test03_CallEventWithNativeInterface_Parms;
	Test03_CallEventWithNativeInterface_Parms.InterfaceParm = InterfaceParm;

	this->ProcessEvent ( pFnTest03_CallEventWithNativeInterface, &Test03_CallEventWithNativeInterface_Parms, NULL );
};

// Function UnrealScriptTest.Test0010_NativeObject.Test02_PassNativeInterfaceToNativeFunction
// [0x00020401] ( FUNC_Final | FUNC_Native )
// Parameters infos:
// class UTest0002_InterfaceNative* InterfaceParm                  ( CPF_Parm )

void UTest0010_NativeObject::Test02_PassNativeInterfaceToNativeFunction ( class UTest0002_InterfaceNative* InterfaceParm )
{
	static UFunction* pFnTest02_PassNativeInterfaceToNativeFunction = NULL;

	if ( ! pFnTest02_PassNativeInterfaceToNativeFunction )
		pFnTest02_PassNativeInterfaceToNativeFunction = (UFunction*) UObject::GObjObjects()->Data[ 30691 ];

	UTest0010_NativeObject_execTest02_PassNativeInterfaceToNativeFunction_Parms Test02_PassNativeInterfaceToNativeFunction_Parms;
	Test02_PassNativeInterfaceToNativeFunction_Parms.InterfaceParm = InterfaceParm;

	pFnTest02_PassNativeInterfaceToNativeFunction->FunctionFlags |= ~0x400;

	this->ProcessEvent ( pFnTest02_PassNativeInterfaceToNativeFunction, &Test02_PassNativeInterfaceToNativeFunction_Parms, NULL );

	pFnTest02_PassNativeInterfaceToNativeFunction->FunctionFlags |= 0x400;
};

// Function UnrealScriptTest.Test0010_NativeObject.Test01_CallEventWithStruct
// [0x00020802] ( FUNC_Event )
// Parameters infos:
// struct FNoCtorProps            NoCtor                         ( CPF_Parm )
// struct FConstructorStructString StringParm                     ( CPF_Parm | CPF_NeedCtorLink )
// struct FConstructorStructArray ArrayParm                      ( CPF_Parm | CPF_NeedCtorLink )
// struct FConstructorStructCombo ComboParm                      ( CPF_Parm | CPF_NeedCtorLink )
// unsigned long                  PaddingBool                    ( CPF_Parm )

void UTest0010_NativeObject::eventTest01_CallEventWithStruct ( struct FNoCtorProps NoCtor, struct FConstructorStructString StringParm, struct FConstructorStructArray ArrayParm, struct FConstructorStructCombo ComboParm, unsigned long PaddingBool )
{
	static UFunction* pFnTest01_CallEventWithStruct = NULL;

	if ( ! pFnTest01_CallEventWithStruct )
		pFnTest01_CallEventWithStruct = (UFunction*) UObject::GObjObjects()->Data[ 30685 ];

	UTest0010_NativeObject_eventTest01_CallEventWithStruct_Parms Test01_CallEventWithStruct_Parms;
	memcpy ( &Test01_CallEventWithStruct_Parms.NoCtor, &NoCtor, 0xC );
	memcpy ( &Test01_CallEventWithStruct_Parms.StringParm, &StringParm, 0x18 );
	memcpy ( &Test01_CallEventWithStruct_Parms.ArrayParm, &ArrayParm, 0x10 );
	memcpy ( &Test01_CallEventWithStruct_Parms.ComboParm, &ComboParm, 0x20 );
	Test01_CallEventWithStruct_Parms.PaddingBool = PaddingBool;

	this->ProcessEvent ( pFnTest01_CallEventWithStruct, &Test01_CallEventWithStruct_Parms, NULL );
};

// Function UnrealScriptTest.Test0010_NativeObject.PerformNativeTest
// [0x00020401] ( FUNC_Final | FUNC_Native )
// Parameters infos:
// int                            TestNumber                     ( CPF_Parm )

void UTest0010_NativeObject::PerformNativeTest ( int TestNumber )
{
	static UFunction* pFnPerformNativeTest = NULL;

	if ( ! pFnPerformNativeTest )
		pFnPerformNativeTest = (UFunction*) UObject::GObjObjects()->Data[ 30646 ];

	UTest0010_NativeObject_execPerformNativeTest_Parms PerformNativeTest_Parms;
	PerformNativeTest_Parms.TestNumber = TestNumber;

	pFnPerformNativeTest->FunctionFlags |= ~0x400;

	this->ProcessEvent ( pFnPerformNativeTest, &PerformNativeTest_Parms, NULL );

	pFnPerformNativeTest->FunctionFlags |= 0x400;
};

// Function UnrealScriptTest.Test0011_NativeObjectBoolOrder.NativeTestBoolOrderEvent
// [0x00020800] ( FUNC_Event )
// Parameters infos:
// unsigned long                  EventBoolParm                  ( CPF_Parm )

void UTest0011_NativeObjectBoolOrder::eventNativeTestBoolOrderEvent ( unsigned long EventBoolParm )
{
	static UFunction* pFnNativeTestBoolOrderEvent = NULL;

	if ( ! pFnNativeTestBoolOrderEvent )
		pFnNativeTestBoolOrderEvent = (UFunction*) UObject::GObjObjects()->Data[ 30732 ];

	UTest0011_NativeObjectBoolOrder_eventNativeTestBoolOrderEvent_Parms NativeTestBoolOrderEvent_Parms;
	NativeTestBoolOrderEvent_Parms.EventBoolParm = EventBoolParm;

	this->ProcessEvent ( pFnNativeTestBoolOrderEvent, &NativeTestBoolOrderEvent_Parms, NULL );
};

// Function UnrealScriptTest.Test0011_NativeObjectBoolOrder.NativeTestBoolOrderFunction
// [0x00020400] ( FUNC_Native )
// Parameters infos:
// bool                           ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// unsigned long                  BoolParm                       ( CPF_Parm )

bool UTest0011_NativeObjectBoolOrder::NativeTestBoolOrderFunction ( unsigned long BoolParm )
{
	static UFunction* pFnNativeTestBoolOrderFunction = NULL;

	if ( ! pFnNativeTestBoolOrderFunction )
		pFnNativeTestBoolOrderFunction = (UFunction*) UObject::GObjObjects()->Data[ 30729 ];

	UTest0011_NativeObjectBoolOrder_execNativeTestBoolOrderFunction_Parms NativeTestBoolOrderFunction_Parms;
	NativeTestBoolOrderFunction_Parms.BoolParm = BoolParm;

	pFnNativeTestBoolOrderFunction->FunctionFlags |= ~0x400;

	this->ProcessEvent ( pFnNativeTestBoolOrderFunction, &NativeTestBoolOrderFunction_Parms, NULL );

	pFnNativeTestBoolOrderFunction->FunctionFlags |= 0x400;

	return NativeTestBoolOrderFunction_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0011_NativeObjectBoolOrder.PerformBoolOrderTest
// [0x00020400] ( FUNC_Native )
// Parameters infos:

void UTest0011_NativeObjectBoolOrder::PerformBoolOrderTest ( )
{
	static UFunction* pFnPerformBoolOrderTest = NULL;

	if ( ! pFnPerformBoolOrderTest )
		pFnPerformBoolOrderTest = (UFunction*) UObject::GObjObjects()->Data[ 30728 ];

	UTest0011_NativeObjectBoolOrder_execPerformBoolOrderTest_Parms PerformBoolOrderTest_Parms;

	pFnPerformBoolOrderTest->FunctionFlags |= ~0x400;

	this->ProcessEvent ( pFnPerformBoolOrderTest, &PerformBoolOrderTest_Parms, NULL );

	pFnPerformBoolOrderTest->FunctionFlags |= 0x400;
};

// Function UnrealScriptTest.Test0025_NativeIssues.RunTest
// [0x00020002] 
// Parameters infos:

void UTest0025_NativeIssues::RunTest ( )
{
	static UFunction* pFnRunTest = NULL;

	if ( ! pFnRunTest )
		pFnRunTest = (UFunction*) UObject::GObjObjects()->Data[ 30959 ];

	UTest0025_NativeIssues_execRunTest_Parms RunTest_Parms;

	this->ProcessEvent ( pFnRunTest, &RunTest_Parms, NULL );
};

// Function UnrealScriptTest.Test0025_NativeIssues.ShowNativeBoolAssignmentTestResults
// [0x00020401] ( FUNC_Final | FUNC_Native )
// Parameters infos:

void UTest0025_NativeIssues::ShowNativeBoolAssignmentTestResults ( )
{
	static UFunction* pFnShowNativeBoolAssignmentTestResults = NULL;

	if ( ! pFnShowNativeBoolAssignmentTestResults )
		pFnShowNativeBoolAssignmentTestResults = (UFunction*) UObject::GObjObjects()->Data[ 30958 ];

	UTest0025_NativeIssues_execShowNativeBoolAssignmentTestResults_Parms ShowNativeBoolAssignmentTestResults_Parms;

	pFnShowNativeBoolAssignmentTestResults->FunctionFlags |= ~0x400;

	this->ProcessEvent ( pFnShowNativeBoolAssignmentTestResults, &ShowNativeBoolAssignmentTestResults_Parms, NULL );

	pFnShowNativeBoolAssignmentTestResults->FunctionFlags |= 0x400;
};

// Function UnrealScriptTest.Test0003_Commandlet.Test0003
// [0x00020002] 
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

int UTest0003_Commandlet::Test0003 ( )
{
	static UFunction* pFnTest0003 = NULL;

	if ( ! pFnTest0003 )
		pFnTest0003 = (UFunction*) UObject::GObjObjects()->Data[ 30541 ];

	UTest0003_Commandlet_execTest0003_Parms Test0003_Parms;

	this->ProcessEvent ( pFnTest0003, &Test0003_Parms, NULL );

	return Test0003_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0003_Commandlet.Main
// [0x00020802] ( FUNC_Event )
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// struct FString                 Params                         ( CPF_Parm | CPF_NeedCtorLink )

int UTest0003_Commandlet::eventMain ( struct FString Params )
{
	static UFunction* pFnMain = NULL;

	if ( ! pFnMain )
		pFnMain = (UFunction*) UObject::GObjObjects()->Data[ 30537 ];

	UTest0003_Commandlet_eventMain_Parms Main_Parms;
	memcpy ( &Main_Parms.Params, &Params, 0xC );

	this->ProcessEvent ( pFnMain, &Main_Parms, NULL );

	return Main_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0003_Interface.Test0003_FunctionF
// [0x00020000] 
// Parameters infos:
// unsigned char                  ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

unsigned char UTest0003_Interface::Test0003_FunctionF ( )
{
	static UFunction* pFnTest0003_FunctionF = NULL;

	if ( ! pFnTest0003_FunctionF )
		pFnTest0003_FunctionF = (UFunction*) UObject::GObjObjects()->Data[ 30570 ];

	UTest0003_Interface_execTest0003_FunctionF_Parms Test0003_FunctionF_Parms;

	this->ProcessEvent ( pFnTest0003_FunctionF, &Test0003_FunctionF_Parms, NULL );

	return Test0003_FunctionF_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0003_Interface.Test0003_FunctionE
// [0x00020000] 
// Parameters infos:
// float                          ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

float UTest0003_Interface::Test0003_FunctionE ( )
{
	static UFunction* pFnTest0003_FunctionE = NULL;

	if ( ! pFnTest0003_FunctionE )
		pFnTest0003_FunctionE = (UFunction*) UObject::GObjObjects()->Data[ 30568 ];

	UTest0003_Interface_execTest0003_FunctionE_Parms Test0003_FunctionE_Parms;

	this->ProcessEvent ( pFnTest0003_FunctionE, &Test0003_FunctionE_Parms, NULL );

	return Test0003_FunctionE_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0003_Interface.Test0003_FunctionD
// [0x00020000] 
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

int UTest0003_Interface::Test0003_FunctionD ( )
{
	static UFunction* pFnTest0003_FunctionD = NULL;

	if ( ! pFnTest0003_FunctionD )
		pFnTest0003_FunctionD = (UFunction*) UObject::GObjObjects()->Data[ 30566 ];

	UTest0003_Interface_execTest0003_FunctionD_Parms Test0003_FunctionD_Parms;

	this->ProcessEvent ( pFnTest0003_FunctionD, &Test0003_FunctionD_Parms, NULL );

	return Test0003_FunctionD_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0003_Interface.Test0003_FunctionC
// [0x00020000] 
// Parameters infos:
// struct FString                 ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )

struct FString UTest0003_Interface::Test0003_FunctionC ( )
{
	static UFunction* pFnTest0003_FunctionC = NULL;

	if ( ! pFnTest0003_FunctionC )
		pFnTest0003_FunctionC = (UFunction*) UObject::GObjObjects()->Data[ 30564 ];

	UTest0003_Interface_execTest0003_FunctionC_Parms Test0003_FunctionC_Parms;

	this->ProcessEvent ( pFnTest0003_FunctionC, &Test0003_FunctionC_Parms, NULL );

	return Test0003_FunctionC_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0003_Interface.Test0003_FunctionB
// [0x00020000] 
// Parameters infos:
// bool                           ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

bool UTest0003_Interface::Test0003_FunctionB ( )
{
	static UFunction* pFnTest0003_FunctionB = NULL;

	if ( ! pFnTest0003_FunctionB )
		pFnTest0003_FunctionB = (UFunction*) UObject::GObjObjects()->Data[ 30562 ];

	UTest0003_Interface_execTest0003_FunctionB_Parms Test0003_FunctionB_Parms;

	this->ProcessEvent ( pFnTest0003_FunctionB, &Test0003_FunctionB_Parms, NULL );

	return Test0003_FunctionB_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0003_Interface.Test0003_FunctionA
// [0x00020000] 
// Parameters infos:

void UTest0003_Interface::Test0003_FunctionA ( )
{
	static UFunction* pFnTest0003_FunctionA = NULL;

	if ( ! pFnTest0003_FunctionA )
		pFnTest0003_FunctionA = (UFunction*) UObject::GObjObjects()->Data[ 30561 ];

	UTest0003_Interface_execTest0003_FunctionA_Parms Test0003_FunctionA_Parms;

	this->ProcessEvent ( pFnTest0003_FunctionA, &Test0003_FunctionA_Parms, NULL );
};

// Function UnrealScriptTest.Test0003_DerivedClass.Test0003_FunctionF
// [0x00020002] 
// Parameters infos:
// unsigned char                  ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

unsigned char UTest0003_DerivedClass::Test0003_FunctionF ( )
{
	static UFunction* pFnTest0003_FunctionF = NULL;

	if ( ! pFnTest0003_FunctionF )
		pFnTest0003_FunctionF = (UFunction*) UObject::GObjObjects()->Data[ 30558 ];

	UTest0003_DerivedClass_execTest0003_FunctionF_Parms Test0003_FunctionF_Parms;

	this->ProcessEvent ( pFnTest0003_FunctionF, &Test0003_FunctionF_Parms, NULL );

	return Test0003_FunctionF_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0003_DerivedClass.Test0003_FunctionE
// [0x00020002] 
// Parameters infos:
// float                          ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

float UTest0003_DerivedClass::Test0003_FunctionE ( )
{
	static UFunction* pFnTest0003_FunctionE = NULL;

	if ( ! pFnTest0003_FunctionE )
		pFnTest0003_FunctionE = (UFunction*) UObject::GObjObjects()->Data[ 30556 ];

	UTest0003_DerivedClass_execTest0003_FunctionE_Parms Test0003_FunctionE_Parms;

	this->ProcessEvent ( pFnTest0003_FunctionE, &Test0003_FunctionE_Parms, NULL );

	return Test0003_FunctionE_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0003_DerivedClass.Test0003_FunctionD
// [0x00020002] 
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

int UTest0003_DerivedClass::Test0003_FunctionD ( )
{
	static UFunction* pFnTest0003_FunctionD = NULL;

	if ( ! pFnTest0003_FunctionD )
		pFnTest0003_FunctionD = (UFunction*) UObject::GObjObjects()->Data[ 30554 ];

	UTest0003_DerivedClass_execTest0003_FunctionD_Parms Test0003_FunctionD_Parms;

	this->ProcessEvent ( pFnTest0003_FunctionD, &Test0003_FunctionD_Parms, NULL );

	return Test0003_FunctionD_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0003_DerivedClass.Test0003_FunctionC
// [0x00020002] 
// Parameters infos:
// struct FString                 ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )

struct FString UTest0003_DerivedClass::Test0003_FunctionC ( )
{
	static UFunction* pFnTest0003_FunctionC = NULL;

	if ( ! pFnTest0003_FunctionC )
		pFnTest0003_FunctionC = (UFunction*) UObject::GObjObjects()->Data[ 30552 ];

	UTest0003_DerivedClass_execTest0003_FunctionC_Parms Test0003_FunctionC_Parms;

	this->ProcessEvent ( pFnTest0003_FunctionC, &Test0003_FunctionC_Parms, NULL );

	return Test0003_FunctionC_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0003_DerivedClass.Test0003_FunctionB
// [0x00020002] 
// Parameters infos:
// bool                           ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

bool UTest0003_DerivedClass::Test0003_FunctionB ( )
{
	static UFunction* pFnTest0003_FunctionB = NULL;

	if ( ! pFnTest0003_FunctionB )
		pFnTest0003_FunctionB = (UFunction*) UObject::GObjObjects()->Data[ 30550 ];

	UTest0003_DerivedClass_execTest0003_FunctionB_Parms Test0003_FunctionB_Parms;

	this->ProcessEvent ( pFnTest0003_FunctionB, &Test0003_FunctionB_Parms, NULL );

	return Test0003_FunctionB_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0003_DerivedClass.Test0003_FunctionA
// [0x00020002] 
// Parameters infos:

void UTest0003_DerivedClass::Test0003_FunctionA ( )
{
	static UFunction* pFnTest0003_FunctionA = NULL;

	if ( ! pFnTest0003_FunctionA )
		pFnTest0003_FunctionA = (UFunction*) UObject::GObjObjects()->Data[ 30549 ];

	UTest0003_DerivedClass_execTest0003_FunctionA_Parms Test0003_FunctionA_Parms;

	this->ProcessEvent ( pFnTest0003_FunctionA, &Test0003_FunctionA_Parms, NULL );
};

// Function UnrealScriptTest.Test0004_BaseClass.Test0004_FunctionF
// [0x00020002] 
// Parameters infos:
// unsigned char                  ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

unsigned char UTest0004_BaseClass::Test0004_FunctionF ( )
{
	static UFunction* pFnTest0004_FunctionF = NULL;

	if ( ! pFnTest0004_FunctionF )
		pFnTest0004_FunctionF = (UFunction*) UObject::GObjObjects()->Data[ 30583 ];

	UTest0004_BaseClass_execTest0004_FunctionF_Parms Test0004_FunctionF_Parms;

	this->ProcessEvent ( pFnTest0004_FunctionF, &Test0004_FunctionF_Parms, NULL );

	return Test0004_FunctionF_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0004_BaseClass.Test0004_FunctionE
// [0x00020002] 
// Parameters infos:
// float                          ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

float UTest0004_BaseClass::Test0004_FunctionE ( )
{
	static UFunction* pFnTest0004_FunctionE = NULL;

	if ( ! pFnTest0004_FunctionE )
		pFnTest0004_FunctionE = (UFunction*) UObject::GObjObjects()->Data[ 30581 ];

	UTest0004_BaseClass_execTest0004_FunctionE_Parms Test0004_FunctionE_Parms;

	this->ProcessEvent ( pFnTest0004_FunctionE, &Test0004_FunctionE_Parms, NULL );

	return Test0004_FunctionE_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0004_BaseClass.Test0004_FunctionD
// [0x00020002] 
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

int UTest0004_BaseClass::Test0004_FunctionD ( )
{
	static UFunction* pFnTest0004_FunctionD = NULL;

	if ( ! pFnTest0004_FunctionD )
		pFnTest0004_FunctionD = (UFunction*) UObject::GObjObjects()->Data[ 30579 ];

	UTest0004_BaseClass_execTest0004_FunctionD_Parms Test0004_FunctionD_Parms;

	this->ProcessEvent ( pFnTest0004_FunctionD, &Test0004_FunctionD_Parms, NULL );

	return Test0004_FunctionD_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0004_BaseClass.Test0004_FunctionC
// [0x00020002] 
// Parameters infos:
// struct FString                 ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )

struct FString UTest0004_BaseClass::Test0004_FunctionC ( )
{
	static UFunction* pFnTest0004_FunctionC = NULL;

	if ( ! pFnTest0004_FunctionC )
		pFnTest0004_FunctionC = (UFunction*) UObject::GObjObjects()->Data[ 30577 ];

	UTest0004_BaseClass_execTest0004_FunctionC_Parms Test0004_FunctionC_Parms;

	this->ProcessEvent ( pFnTest0004_FunctionC, &Test0004_FunctionC_Parms, NULL );

	return Test0004_FunctionC_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0004_BaseClass.Test0004_FunctionB
// [0x00020002] 
// Parameters infos:
// bool                           ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

bool UTest0004_BaseClass::Test0004_FunctionB ( )
{
	static UFunction* pFnTest0004_FunctionB = NULL;

	if ( ! pFnTest0004_FunctionB )
		pFnTest0004_FunctionB = (UFunction*) UObject::GObjObjects()->Data[ 30575 ];

	UTest0004_BaseClass_execTest0004_FunctionB_Parms Test0004_FunctionB_Parms;

	this->ProcessEvent ( pFnTest0004_FunctionB, &Test0004_FunctionB_Parms, NULL );

	return Test0004_FunctionB_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0004_BaseClass.Test0004_FunctionA
// [0x00020002] 
// Parameters infos:

void UTest0004_BaseClass::Test0004_FunctionA ( )
{
	static UFunction* pFnTest0004_FunctionA = NULL;

	if ( ! pFnTest0004_FunctionA )
		pFnTest0004_FunctionA = (UFunction*) UObject::GObjObjects()->Data[ 30574 ];

	UTest0004_BaseClass_execTest0004_FunctionA_Parms Test0004_FunctionA_Parms;

	this->ProcessEvent ( pFnTest0004_FunctionA, &Test0004_FunctionA_Parms, NULL );
};

// Function UnrealScriptTest.Test0004_Interface.Test0004_FunctionF
// [0x00020000] 
// Parameters infos:
// unsigned char                  ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

unsigned char UTest0004_Interface::Test0004_FunctionF ( )
{
	static UFunction* pFnTest0004_FunctionF = NULL;

	if ( ! pFnTest0004_FunctionF )
		pFnTest0004_FunctionF = (UFunction*) UObject::GObjObjects()->Data[ 30599 ];

	UTest0004_Interface_execTest0004_FunctionF_Parms Test0004_FunctionF_Parms;

	this->ProcessEvent ( pFnTest0004_FunctionF, &Test0004_FunctionF_Parms, NULL );

	return Test0004_FunctionF_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0004_Interface.Test0004_FunctionE
// [0x00020000] 
// Parameters infos:
// float                          ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

float UTest0004_Interface::Test0004_FunctionE ( )
{
	static UFunction* pFnTest0004_FunctionE = NULL;

	if ( ! pFnTest0004_FunctionE )
		pFnTest0004_FunctionE = (UFunction*) UObject::GObjObjects()->Data[ 30597 ];

	UTest0004_Interface_execTest0004_FunctionE_Parms Test0004_FunctionE_Parms;

	this->ProcessEvent ( pFnTest0004_FunctionE, &Test0004_FunctionE_Parms, NULL );

	return Test0004_FunctionE_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0004_Interface.Test0004_FunctionD
// [0x00020000] 
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

int UTest0004_Interface::Test0004_FunctionD ( )
{
	static UFunction* pFnTest0004_FunctionD = NULL;

	if ( ! pFnTest0004_FunctionD )
		pFnTest0004_FunctionD = (UFunction*) UObject::GObjObjects()->Data[ 30595 ];

	UTest0004_Interface_execTest0004_FunctionD_Parms Test0004_FunctionD_Parms;

	this->ProcessEvent ( pFnTest0004_FunctionD, &Test0004_FunctionD_Parms, NULL );

	return Test0004_FunctionD_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0004_Interface.Test0004_FunctionC
// [0x00020000] 
// Parameters infos:
// struct FString                 ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )

struct FString UTest0004_Interface::Test0004_FunctionC ( )
{
	static UFunction* pFnTest0004_FunctionC = NULL;

	if ( ! pFnTest0004_FunctionC )
		pFnTest0004_FunctionC = (UFunction*) UObject::GObjObjects()->Data[ 30593 ];

	UTest0004_Interface_execTest0004_FunctionC_Parms Test0004_FunctionC_Parms;

	this->ProcessEvent ( pFnTest0004_FunctionC, &Test0004_FunctionC_Parms, NULL );

	return Test0004_FunctionC_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0004_Interface.Test0004_FunctionB
// [0x00020000] 
// Parameters infos:
// bool                           ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

bool UTest0004_Interface::Test0004_FunctionB ( )
{
	static UFunction* pFnTest0004_FunctionB = NULL;

	if ( ! pFnTest0004_FunctionB )
		pFnTest0004_FunctionB = (UFunction*) UObject::GObjObjects()->Data[ 30591 ];

	UTest0004_Interface_execTest0004_FunctionB_Parms Test0004_FunctionB_Parms;

	this->ProcessEvent ( pFnTest0004_FunctionB, &Test0004_FunctionB_Parms, NULL );

	return Test0004_FunctionB_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0004_Interface.Test0004_FunctionA
// [0x00020000] 
// Parameters infos:

void UTest0004_Interface::Test0004_FunctionA ( )
{
	static UFunction* pFnTest0004_FunctionA = NULL;

	if ( ! pFnTest0004_FunctionA )
		pFnTest0004_FunctionA = (UFunction*) UObject::GObjObjects()->Data[ 30590 ];

	UTest0004_Interface_execTest0004_FunctionA_Parms Test0004_FunctionA_Parms;

	this->ProcessEvent ( pFnTest0004_FunctionA, &Test0004_FunctionA_Parms, NULL );
};

// Function UnrealScriptTest.Test0004_DerivedClass.Test0004_FunctionA
// [0x00020002] 
// Parameters infos:

void UTest0004_DerivedClass::Test0004_FunctionA ( )
{
	static UFunction* pFnTest0004_FunctionA = NULL;

	if ( ! pFnTest0004_FunctionA )
		pFnTest0004_FunctionA = (UFunction*) UObject::GObjObjects()->Data[ 30588 ];

	UTest0004_DerivedClass_execTest0004_FunctionA_Parms Test0004_FunctionA_Parms;

	this->ProcessEvent ( pFnTest0004_FunctionA, &Test0004_FunctionA_Parms, NULL );
};

// Function UnrealScriptTest.Test0005_Commandlet.Test0005
// [0x00020002] 
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

int UTest0005_Commandlet::Test0005 ( )
{
	static UFunction* pFnTest0005 = NULL;

	if ( ! pFnTest0005 )
		pFnTest0005 = (UFunction*) UObject::GObjObjects()->Data[ 30607 ];

	UTest0005_Commandlet_execTest0005_Parms Test0005_Parms;

	this->ProcessEvent ( pFnTest0005, &Test0005_Parms, NULL );

	return Test0005_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0005_Commandlet.Main
// [0x00020802] ( FUNC_Event )
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// struct FString                 Params                         ( CPF_Parm | CPF_NeedCtorLink )

int UTest0005_Commandlet::eventMain ( struct FString Params )
{
	static UFunction* pFnMain = NULL;

	if ( ! pFnMain )
		pFnMain = (UFunction*) UObject::GObjObjects()->Data[ 30603 ];

	UTest0005_Commandlet_eventMain_Parms Main_Parms;
	memcpy ( &Main_Parms.Params, &Params, 0xC );

	this->ProcessEvent ( pFnMain, &Main_Parms, NULL );

	return Main_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0005_ImplementingClass.derivedFoo
// [0x00020002] 
// Parameters infos:
// struct FString                 ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )

struct FString UTest0005_ImplementingClass::derivedFoo ( )
{
	static UFunction* pFnderivedFoo = NULL;

	if ( ! pFnderivedFoo )
		pFnderivedFoo = (UFunction*) UObject::GObjObjects()->Data[ 30618 ];

	UTest0005_ImplementingClass_execderivedFoo_Parms derivedFoo_Parms;

	this->ProcessEvent ( pFnderivedFoo, &derivedFoo_Parms, NULL );

	return derivedFoo_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0005_ImplementingClass.baseFoo
// [0x00020002] 
// Parameters infos:
// struct FString                 ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )

struct FString UTest0005_ImplementingClass::baseFoo ( )
{
	static UFunction* pFnbaseFoo = NULL;

	if ( ! pFnbaseFoo )
		pFnbaseFoo = (UFunction*) UObject::GObjObjects()->Data[ 30616 ];

	UTest0005_ImplementingClass_execbaseFoo_Parms baseFoo_Parms;

	this->ProcessEvent ( pFnbaseFoo, &baseFoo_Parms, NULL );

	return baseFoo_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0005_InterfaceDerived.derivedFoo
// [0x00020000] 
// Parameters infos:
// struct FString                 ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )

struct FString UTest0005_InterfaceDerived::derivedFoo ( )
{
	static UFunction* pFnderivedFoo = NULL;

	if ( ! pFnderivedFoo )
		pFnderivedFoo = (UFunction*) UObject::GObjObjects()->Data[ 30625 ];

	UTest0005_InterfaceDerived_execderivedFoo_Parms derivedFoo_Parms;

	this->ProcessEvent ( pFnderivedFoo, &derivedFoo_Parms, NULL );

	return derivedFoo_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0005_InterfaceBase.baseFoo
// [0x00020000] 
// Parameters infos:
// struct FString                 ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm | CPF_NeedCtorLink )

struct FString UTest0005_InterfaceBase::baseFoo ( )
{
	static UFunction* pFnbaseFoo = NULL;

	if ( ! pFnbaseFoo )
		pFnbaseFoo = (UFunction*) UObject::GObjObjects()->Data[ 30622 ];

	UTest0005_InterfaceBase_execbaseFoo_Parms baseFoo_Parms;

	this->ProcessEvent ( pFnbaseFoo, &baseFoo_Parms, NULL );

	return baseFoo_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0010_Commandlet.TestInterface
// [0x00020000] 
// Parameters infos:

void UTest0010_Commandlet::TestInterface ( )
{
	static UFunction* pFnTestInterface = NULL;

	if ( ! pFnTestInterface )
		pFnTestInterface = (UFunction*) UObject::GObjObjects()->Data[ 30647 ];

	UTest0010_Commandlet_execTestInterface_Parms TestInterface_Parms;

	this->ProcessEvent ( pFnTestInterface, &TestInterface_Parms, NULL );
};

// Function UnrealScriptTest.Test0010_Commandlet.Test0010
// [0x00020002] 
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// int                            TestNumber                     ( CPF_Parm | CPF_CoerceParm )

int UTest0010_Commandlet::Test0010 ( int TestNumber )
{
	static UFunction* pFnTest0010 = NULL;

	if ( ! pFnTest0010 )
		pFnTest0010 = (UFunction*) UObject::GObjObjects()->Data[ 30641 ];

	UTest0010_Commandlet_execTest0010_Parms Test0010_Parms;
	Test0010_Parms.TestNumber = TestNumber;

	this->ProcessEvent ( pFnTest0010, &Test0010_Parms, NULL );

	return Test0010_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0010_Commandlet.Main
// [0x00020802] ( FUNC_Event )
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// struct FString                 Params                         ( CPF_Parm | CPF_NeedCtorLink )

int UTest0010_Commandlet::eventMain ( struct FString Params )
{
	static UFunction* pFnMain = NULL;

	if ( ! pFnMain )
		pFnMain = (UFunction*) UObject::GObjObjects()->Data[ 30637 ];

	UTest0010_Commandlet_eventMain_Parms Main_Parms;
	memcpy ( &Main_Parms.Params, &Params, 0xC );

	this->ProcessEvent ( pFnMain, &Main_Parms, NULL );

	return Main_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0011_Commandlet.PerformBoolOrderTest
// [0x00020003] ( FUNC_Final )
// Parameters infos:

void UTest0011_Commandlet::PerformBoolOrderTest ( )
{
	static UFunction* pFnPerformBoolOrderTest = NULL;

	if ( ! pFnPerformBoolOrderTest )
		pFnPerformBoolOrderTest = (UFunction*) UObject::GObjObjects()->Data[ 30722 ];

	UTest0011_Commandlet_execPerformBoolOrderTest_Parms PerformBoolOrderTest_Parms;

	this->ProcessEvent ( pFnPerformBoolOrderTest, &PerformBoolOrderTest_Parms, NULL );
};

// Function UnrealScriptTest.Test0011_Commandlet.Main
// [0x00020802] ( FUNC_Event )
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// struct FString                 Parms                          ( CPF_Parm | CPF_NeedCtorLink )

int UTest0011_Commandlet::eventMain ( struct FString Parms )
{
	static UFunction* pFnMain = NULL;

	if ( ! pFnMain )
		pFnMain = (UFunction*) UObject::GObjObjects()->Data[ 30719 ];

	UTest0011_Commandlet_eventMain_Parms Main_Parms;
	memcpy ( &Main_Parms.Parms, &Parms, 0xC );

	this->ProcessEvent ( pFnMain, &Main_Parms, NULL );

	return Main_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0012_IfVarEqualEqualEnum.VarEqualEqualEnumTypeEnum
// [0x00020002] 
// Parameters infos:

void UTest0012_IfVarEqualEqualEnum::VarEqualEqualEnumTypeEnum ( )
{
	static UFunction* pFnVarEqualEqualEnumTypeEnum = NULL;

	if ( ! pFnVarEqualEqualEnumTypeEnum )
		pFnVarEqualEqualEnumTypeEnum = (UFunction*) UObject::GObjObjects()->Data[ 30738 ];

	UTest0012_IfVarEqualEqualEnum_execVarEqualEqualEnumTypeEnum_Parms VarEqualEqualEnumTypeEnum_Parms;

	this->ProcessEvent ( pFnVarEqualEqualEnumTypeEnum, &VarEqualEqualEnumTypeEnum_Parms, NULL );
};

// Function UnrealScriptTest.Test0012_IfVarEqualEqualEnum.VarEqualEqualEnum
// [0x00020002] 
// Parameters infos:

void UTest0012_IfVarEqualEqualEnum::VarEqualEqualEnum ( )
{
	static UFunction* pFnVarEqualEqualEnum = NULL;

	if ( ! pFnVarEqualEqualEnum )
		pFnVarEqualEqualEnum = (UFunction*) UObject::GObjObjects()->Data[ 30736 ];

	UTest0012_IfVarEqualEqualEnum_execVarEqualEqualEnum_Parms VarEqualEqualEnum_Parms;

	this->ProcessEvent ( pFnVarEqualEqualEnum, &VarEqualEqualEnum_Parms, NULL );
};

// Function UnrealScriptTest.Test0012_IfVarEqualEqualEnumDerivedClass.VarEqualEqualEnumTypeEnum
// [0x00020000] 
// Parameters infos:

void UTest0012_IfVarEqualEqualEnumDerivedClass::VarEqualEqualEnumTypeEnum ( )
{
	static UFunction* pFnVarEqualEqualEnumTypeEnum = NULL;

	if ( ! pFnVarEqualEqualEnumTypeEnum )
		pFnVarEqualEqualEnumTypeEnum = (UFunction*) UObject::GObjObjects()->Data[ 30744 ];

	UTest0012_IfVarEqualEqualEnumDerivedClass_execVarEqualEqualEnumTypeEnum_Parms VarEqualEqualEnumTypeEnum_Parms;

	this->ProcessEvent ( pFnVarEqualEqualEnumTypeEnum, &VarEqualEqualEnumTypeEnum_Parms, NULL );
};

// Function UnrealScriptTest.Test0012_IfVarEqualEqualEnumDerivedClass.VarEqualEqualEnum
// [0x00020002] 
// Parameters infos:

void UTest0012_IfVarEqualEqualEnumDerivedClass::VarEqualEqualEnum ( )
{
	static UFunction* pFnVarEqualEqualEnum = NULL;

	if ( ! pFnVarEqualEqualEnum )
		pFnVarEqualEqualEnum = (UFunction*) UObject::GObjObjects()->Data[ 30742 ];

	UTest0012_IfVarEqualEqualEnumDerivedClass_execVarEqualEqualEnum_Parms VarEqualEqualEnum_Parms;

	this->ProcessEvent ( pFnVarEqualEqualEnum, &VarEqualEqualEnum_Parms, NULL );
};

// Function UnrealScriptTest.Test0013_Commandlet.Main
// [0x00020802] ( FUNC_Event )
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// struct FString                 Parms                          ( CPF_Parm | CPF_NeedCtorLink )

int UTest0013_Commandlet::eventMain ( struct FString Parms )
{
	static UFunction* pFnMain = NULL;

	if ( ! pFnMain )
		pFnMain = (UFunction*) UObject::GObjObjects()->Data[ 30747 ];

	UTest0013_Commandlet_eventMain_Parms Main_Parms;
	memcpy ( &Main_Parms.Parms, &Parms, 0xC );

	this->ProcessEvent ( pFnMain, &Main_Parms, NULL );

	return Main_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0013_IfEnumEqualEqualVar.CompareDifferentEnumTypes
// [0x00020002] 
// Parameters infos:

void UTest0013_IfEnumEqualEqualVar::CompareDifferentEnumTypes ( )
{
	static UFunction* pFnCompareDifferentEnumTypes = NULL;

	if ( ! pFnCompareDifferentEnumTypes )
		pFnCompareDifferentEnumTypes = (UFunction*) UObject::GObjObjects()->Data[ 30764 ];

	UTest0013_IfEnumEqualEqualVar_execCompareDifferentEnumTypes_Parms CompareDifferentEnumTypes_Parms;

	this->ProcessEvent ( pFnCompareDifferentEnumTypes, &CompareDifferentEnumTypes_Parms, NULL );
};

// Function UnrealScriptTest.Test0013_IfEnumEqualEqualVar.EnumTypeEnumEqualEqualVar
// [0x00020002] 
// Parameters infos:

void UTest0013_IfEnumEqualEqualVar::EnumTypeEnumEqualEqualVar ( )
{
	static UFunction* pFnEnumTypeEnumEqualEqualVar = NULL;

	if ( ! pFnEnumTypeEnumEqualEqualVar )
		pFnEnumTypeEnumEqualEqualVar = (UFunction*) UObject::GObjObjects()->Data[ 30762 ];

	UTest0013_IfEnumEqualEqualVar_execEnumTypeEnumEqualEqualVar_Parms EnumTypeEnumEqualEqualVar_Parms;

	this->ProcessEvent ( pFnEnumTypeEnumEqualEqualVar, &EnumTypeEnumEqualEqualVar_Parms, NULL );
};

// Function UnrealScriptTest.Test0013_IfEnumEqualEqualVar.EnumEqualEqualVar
// [0x00020000] 
// Parameters infos:

void UTest0013_IfEnumEqualEqualVar::EnumEqualEqualVar ( )
{
	static UFunction* pFnEnumEqualEqualVar = NULL;

	if ( ! pFnEnumEqualEqualVar )
		pFnEnumEqualEqualVar = (UFunction*) UObject::GObjObjects()->Data[ 30761 ];

	UTest0013_IfEnumEqualEqualVar_execEnumEqualEqualVar_Parms EnumEqualEqualVar_Parms;

	this->ProcessEvent ( pFnEnumEqualEqualVar, &EnumEqualEqualVar_Parms, NULL );
};

// Function UnrealScriptTest.Test0013_IfEnumEqualEqualVarDerivedClass.EnumTypeEnumEqualEqualVar
// [0x00020000] 
// Parameters infos:

void UTest0013_IfEnumEqualEqualVarDerivedClass::EnumTypeEnumEqualEqualVar ( )
{
	static UFunction* pFnEnumTypeEnumEqualEqualVar = NULL;

	if ( ! pFnEnumTypeEnumEqualEqualVar )
		pFnEnumTypeEnumEqualEqualVar = (UFunction*) UObject::GObjObjects()->Data[ 30769 ];

	UTest0013_IfEnumEqualEqualVarDerivedClass_execEnumTypeEnumEqualEqualVar_Parms EnumTypeEnumEqualEqualVar_Parms;

	this->ProcessEvent ( pFnEnumTypeEnumEqualEqualVar, &EnumTypeEnumEqualEqualVar_Parms, NULL );
};

// Function UnrealScriptTest.Test0013_IfEnumEqualEqualVarDerivedClass.EnumEqualEqualVar
// [0x00020000] 
// Parameters infos:

void UTest0013_IfEnumEqualEqualVarDerivedClass::EnumEqualEqualVar ( )
{
	static UFunction* pFnEnumEqualEqualVar = NULL;

	if ( ! pFnEnumEqualEqualVar )
		pFnEnumEqualEqualVar = (UFunction*) UObject::GObjObjects()->Data[ 30768 ];

	UTest0013_IfEnumEqualEqualVarDerivedClass_execEnumEqualEqualVar_Parms EnumEqualEqualVar_Parms;

	this->ProcessEvent ( pFnEnumEqualEqualVar, &EnumEqualEqualVar_Parms, NULL );
};

// Function UnrealScriptTest.Test0014_FullyQualifiedEnumFunction.Caller
// [0x00020002] 
// Parameters infos:

void UTest0014_FullyQualifiedEnumFunction::Caller ( )
{
	static UFunction* pFnCaller = NULL;

	if ( ! pFnCaller )
		pFnCaller = (UFunction*) UObject::GObjObjects()->Data[ 30775 ];

	UTest0014_FullyQualifiedEnumFunction_execCaller_Parms Caller_Parms;

	this->ProcessEvent ( pFnCaller, &Caller_Parms, NULL );
};

// Function UnrealScriptTest.Test0014_FullyQualifiedEnumFunction.FullyQualifiedEnum
// [0x00020000] 
// Parameters infos:
// unsigned char                  val                            ( CPF_Parm )

void UTest0014_FullyQualifiedEnumFunction::FullyQualifiedEnum ( unsigned char val )
{
	static UFunction* pFnFullyQualifiedEnum = NULL;

	if ( ! pFnFullyQualifiedEnum )
		pFnFullyQualifiedEnum = (UFunction*) UObject::GObjObjects()->Data[ 30773 ];

	UTest0014_FullyQualifiedEnumFunction_execFullyQualifiedEnum_Parms FullyQualifiedEnum_Parms;
	FullyQualifiedEnum_Parms.val = val;

	this->ProcessEvent ( pFnFullyQualifiedEnum, &FullyQualifiedEnum_Parms, NULL );
};

// Function UnrealScriptTest.Test0014_FullyQualifiedEnumFunctionDerivedClass.Caller
// [0x00020002] 
// Parameters infos:

void UTest0014_FullyQualifiedEnumFunctionDerivedClass::Caller ( )
{
	static UFunction* pFnCaller = NULL;

	if ( ! pFnCaller )
		pFnCaller = (UFunction*) UObject::GObjObjects()->Data[ 30780 ];

	UTest0014_FullyQualifiedEnumFunctionDerivedClass_execCaller_Parms Caller_Parms;

	this->ProcessEvent ( pFnCaller, &Caller_Parms, NULL );
};

// Function UnrealScriptTest.Test0014_FullyQualifiedEnumFunctionDerivedClass.FullyQualifiedEnum
// [0x00020000] 
// Parameters infos:
// unsigned char                  val                            ( CPF_Parm )

void UTest0014_FullyQualifiedEnumFunctionDerivedClass::FullyQualifiedEnum ( unsigned char val )
{
	static UFunction* pFnFullyQualifiedEnum = NULL;

	if ( ! pFnFullyQualifiedEnum )
		pFnFullyQualifiedEnum = (UFunction*) UObject::GObjObjects()->Data[ 30778 ];

	UTest0014_FullyQualifiedEnumFunctionDerivedClass_execFullyQualifiedEnum_Parms FullyQualifiedEnum_Parms;
	FullyQualifiedEnum_Parms.val = val;

	this->ProcessEvent ( pFnFullyQualifiedEnum, &FullyQualifiedEnum_Parms, NULL );
};

// Function UnrealScriptTest.Test0015_FullyQualifiedEnumSwitchCase.CaseStatementWithFullQualifiedEnum
// [0x00020002] 
// Parameters infos:

void UTest0015_FullyQualifiedEnumSwitchCase::CaseStatementWithFullQualifiedEnum ( )
{
	static UFunction* pFnCaseStatementWithFullQualifiedEnum = NULL;

	if ( ! pFnCaseStatementWithFullQualifiedEnum )
		pFnCaseStatementWithFullQualifiedEnum = (UFunction*) UObject::GObjObjects()->Data[ 30784 ];

	UTest0015_FullyQualifiedEnumSwitchCase_execCaseStatementWithFullQualifiedEnum_Parms CaseStatementWithFullQualifiedEnum_Parms;

	this->ProcessEvent ( pFnCaseStatementWithFullQualifiedEnum, &CaseStatementWithFullQualifiedEnum_Parms, NULL );
};

// Function UnrealScriptTest.Test0015_FullyQualifiedEnumSwitchCaseDerivedClass.CaseStatementWithFullQualifiedEnum
// [0x00020002] 
// Parameters infos:

void UTest0015_FullyQualifiedEnumSwitchCaseDerivedClass::CaseStatementWithFullQualifiedEnum ( )
{
	static UFunction* pFnCaseStatementWithFullQualifiedEnum = NULL;

	if ( ! pFnCaseStatementWithFullQualifiedEnum )
		pFnCaseStatementWithFullQualifiedEnum = (UFunction*) UObject::GObjObjects()->Data[ 30788 ];

	UTest0015_FullyQualifiedEnumSwitchCaseDerivedClass_execCaseStatementWithFullQualifiedEnum_Parms CaseStatementWithFullQualifiedEnum_Parms;

	this->ProcessEvent ( pFnCaseStatementWithFullQualifiedEnum, &CaseStatementWithFullQualifiedEnum_Parms, NULL );
};

// Function UnrealScriptTest.Test0020_Commandlet.PerformStructSerializationTest
// [0x00020003] ( FUNC_Final )
// Parameters infos:

void UTest0020_Commandlet::PerformStructSerializationTest ( )
{
	static UFunction* pFnPerformStructSerializationTest = NULL;

	if ( ! pFnPerformStructSerializationTest )
		pFnPerformStructSerializationTest = (UFunction*) UObject::GObjObjects()->Data[ 30809 ];

	UTest0020_Commandlet_execPerformStructSerializationTest_Parms PerformStructSerializationTest_Parms;

	this->ProcessEvent ( pFnPerformStructSerializationTest, &PerformStructSerializationTest_Parms, NULL );
};

// Function UnrealScriptTest.Test0020_Commandlet.Main
// [0x00020802] ( FUNC_Event )
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// struct FString                 Parms                          ( CPF_Parm | CPF_NeedCtorLink )

int UTest0020_Commandlet::eventMain ( struct FString Parms )
{
	static UFunction* pFnMain = NULL;

	if ( ! pFnMain )
		pFnMain = (UFunction*) UObject::GObjObjects()->Data[ 30806 ];

	UTest0020_Commandlet_eventMain_Parms Main_Parms;
	memcpy ( &Main_Parms.Parms, &Parms, 0xC );

	this->ProcessEvent ( pFnMain, &Main_Parms, NULL );

	return Main_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0020_StructDefaults.LogPropertyValues
// [0x00024001] ( FUNC_Final )
// Parameters infos:
// struct FTestStruct             ParameterStruct                ( CPF_OptionalParm | CPF_Parm )

void UTest0020_StructDefaults::LogPropertyValues ( struct FTestStruct ParameterStruct )
{
	static UFunction* pFnLogPropertyValues = NULL;

	if ( ! pFnLogPropertyValues )
		pFnLogPropertyValues = (UFunction*) UObject::GObjObjects()->Data[ 30812 ];

	UTest0020_StructDefaults_execLogPropertyValues_Parms LogPropertyValues_Parms;
	memcpy ( &LogPropertyValues_Parms.ParameterStruct, &ParameterStruct, 0x20 );

	this->ProcessEvent ( pFnLogPropertyValues, &LogPropertyValues_Parms, NULL );
};

// Function UnrealScriptTest.Test0021_Commandlet.Test0021
// [0x00020002] 
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

int UTest0021_Commandlet::Test0021 ( )
{
	static UFunction* pFnTest0021 = NULL;

	if ( ! pFnTest0021 )
		pFnTest0021 = (UFunction*) UObject::GObjObjects()->Data[ 30842 ];

	UTest0021_Commandlet_execTest0021_Parms Test0021_Parms;

	this->ProcessEvent ( pFnTest0021, &Test0021_Parms, NULL );

	return Test0021_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0021_Commandlet.Main
// [0x00020802] ( FUNC_Event )
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// struct FString                 Params                         ( CPF_Parm | CPF_NeedCtorLink )

int UTest0021_Commandlet::eventMain ( struct FString Params )
{
	static UFunction* pFnMain = NULL;

	if ( ! pFnMain )
		pFnMain = (UFunction*) UObject::GObjObjects()->Data[ 30838 ];

	UTest0021_Commandlet_eventMain_Parms Main_Parms;
	memcpy ( &Main_Parms.Params, &Params, 0xC );

	this->ProcessEvent ( pFnMain, &Main_Parms, NULL );

	return Main_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0021_PrivateFunctionLookupDerived.DifferentNumberOfParams
// [0x00020002] 
// Parameters infos:
// float                          DeltaTime                      ( CPF_Parm )

void UTest0021_PrivateFunctionLookupDerived::DifferentNumberOfParams ( float DeltaTime )
{
	static UFunction* pFnDifferentNumberOfParams = NULL;

	if ( ! pFnDifferentNumberOfParams )
		pFnDifferentNumberOfParams = (UFunction*) UObject::GObjObjects()->Data[ 30855 ];

	UTest0021_PrivateFunctionLookupDerived_execDifferentNumberOfParams_Parms DifferentNumberOfParams_Parms;
	DifferentNumberOfParams_Parms.DeltaTime = DeltaTime;

	this->ProcessEvent ( pFnDifferentNumberOfParams, &DifferentNumberOfParams_Parms, NULL );
};

// Function UnrealScriptTest.Test0021_PrivateFunctionLookupBase.DifferentNumberOfParams
// [0x00040003] ( FUNC_Final )
// Parameters infos:
// float                          DeltaTime                      ( CPF_Parm )
// struct FRotator                CurrentRotation                ( CPF_Parm )

void UTest0021_PrivateFunctionLookupBase::DifferentNumberOfParams ( float DeltaTime, struct FRotator CurrentRotation )
{
	static UFunction* pFnDifferentNumberOfParams = NULL;

	if ( ! pFnDifferentNumberOfParams )
		pFnDifferentNumberOfParams = (UFunction*) UObject::GObjObjects()->Data[ 30851 ];

	UTest0021_PrivateFunctionLookupBase_execDifferentNumberOfParams_Parms DifferentNumberOfParams_Parms;
	DifferentNumberOfParams_Parms.DeltaTime = DeltaTime;
	memcpy ( &DifferentNumberOfParams_Parms.CurrentRotation, &CurrentRotation, 0xC );

	this->ProcessEvent ( pFnDifferentNumberOfParams, &DifferentNumberOfParams_Parms, NULL );
};

// Function UnrealScriptTest.Test0021_PrivateFunctionLookupBase.Test0021
// [0x00820002] 
// Parameters infos:

void UTest0021_PrivateFunctionLookupBase::Test0021 ( )
{
	static UFunction* pFnTest0021 = NULL;

	if ( ! pFnTest0021 )
		pFnTest0021 = (UFunction*) UObject::GObjObjects()->Data[ 30849 ];

	UTest0021_PrivateFunctionLookupBase_execTest0021_Parms Test0021_Parms;

	this->ProcessEvent ( pFnTest0021, &Test0021_Parms, NULL );
};

// Function UnrealScriptTest.Test0022_Commandlet.Main
// [0x00020802] ( FUNC_Event )
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// struct FString                 Parms                          ( CPF_Parm | CPF_NeedCtorLink )

int UTest0022_Commandlet::eventMain ( struct FString Parms )
{
	static UFunction* pFnMain = NULL;

	if ( ! pFnMain )
		pFnMain = (UFunction*) UObject::GObjObjects()->Data[ 30859 ];

	UTest0022_Commandlet_eventMain_Parms Main_Parms;
	memcpy ( &Main_Parms.Parms, &Parms, 0xC );

	this->ProcessEvent ( pFnMain, &Main_Parms, NULL );

	return Main_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.TestInvalidArrayExpansion
// [0x00020002] 
// Parameters infos:

void UTest0022_StructMemberAccess::TestInvalidArrayExpansion ( )
{
	static UFunction* pFnTestInvalidArrayExpansion = NULL;

	if ( ! pFnTestInvalidArrayExpansion )
		pFnTestInvalidArrayExpansion = (UFunction*) UObject::GObjObjects()->Data[ 30904 ];

	UTest0022_StructMemberAccess_execTestInvalidArrayExpansion_Parms TestInvalidArrayExpansion_Parms;

	this->ProcessEvent ( pFnTestInvalidArrayExpansion, &TestInvalidArrayExpansion_Parms, NULL );
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.ReturnInlineStruct
// [0x00040003] ( FUNC_Final )
// Parameters infos:
// struct FVector                 ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

struct FVector UTest0022_StructMemberAccess::ReturnInlineStruct ( )
{
	static UFunction* pFnReturnInlineStruct = NULL;

	if ( ! pFnReturnInlineStruct )
		pFnReturnInlineStruct = (UFunction*) UObject::GObjObjects()->Data[ 30898 ];

	UTest0022_StructMemberAccess_execReturnInlineStruct_Parms ReturnInlineStruct_Parms;

	this->ProcessEvent ( pFnReturnInlineStruct, &ReturnInlineStruct_Parms, NULL );

	return ReturnInlineStruct_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.ReturnMemberStruct
// [0x00040003] ( FUNC_Final )
// Parameters infos:
// struct FOuterStruct            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

struct FOuterStruct UTest0022_StructMemberAccess::ReturnMemberStruct ( )
{
	static UFunction* pFnReturnMemberStruct = NULL;

	if ( ! pFnReturnMemberStruct )
		pFnReturnMemberStruct = (UFunction*) UObject::GObjObjects()->Data[ 30897 ];

	UTest0022_StructMemberAccess_execReturnMemberStruct_Parms ReturnMemberStruct_Parms;

	this->ProcessEvent ( pFnReturnMemberStruct, &ReturnMemberStruct_Parms, NULL );

	return ReturnMemberStruct_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.InlineStructAccessTest
// [0x00020002] 
// Parameters infos:

void UTest0022_StructMemberAccess::InlineStructAccessTest ( )
{
	static UFunction* pFnInlineStructAccessTest = NULL;

	if ( ! pFnInlineStructAccessTest )
		pFnInlineStructAccessTest = (UFunction*) UObject::GObjObjects()->Data[ 30900 ];

	UTest0022_StructMemberAccess_execInlineStructAccessTest_Parms InlineStructAccessTest_Parms;

	this->ProcessEvent ( pFnInlineStructAccessTest, &InlineStructAccessTest_Parms, NULL );
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.DelegateReturnValueAccessTest
// [0x00020002] 
// Parameters infos:

void UTest0022_StructMemberAccess::DelegateReturnValueAccessTest ( )
{
	static UFunction* pFnDelegateReturnValueAccessTest = NULL;

	if ( ! pFnDelegateReturnValueAccessTest )
		pFnDelegateReturnValueAccessTest = (UFunction*) UObject::GObjObjects()->Data[ 30899 ];

	UTest0022_StructMemberAccess_execDelegateReturnValueAccessTest_Parms DelegateReturnValueAccessTest_Parms;

	this->ProcessEvent ( pFnDelegateReturnValueAccessTest, &DelegateReturnValueAccessTest_Parms, NULL );
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.ReturnValueAccessTest
// [0x00820002] 
// Parameters infos:

void UTest0022_StructMemberAccess::ReturnValueAccessTest ( )
{
	static UFunction* pFnReturnValueAccessTest = NULL;

	if ( ! pFnReturnValueAccessTest )
		pFnReturnValueAccessTest = (UFunction*) UObject::GObjObjects()->Data[ 30894 ];

	UTest0022_StructMemberAccess_execReturnValueAccessTest_Parms ReturnValueAccessTest_Parms;

	this->ProcessEvent ( pFnReturnValueAccessTest, &ReturnValueAccessTest_Parms, NULL );
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.ArrayStructAccessTest
// [0x00020002] 
// Parameters infos:

void UTest0022_StructMemberAccess::ArrayStructAccessTest ( )
{
	static UFunction* pFnArrayStructAccessTest = NULL;

	if ( ! pFnArrayStructAccessTest )
		pFnArrayStructAccessTest = (UFunction*) UObject::GObjObjects()->Data[ 30892 ];

	UTest0022_StructMemberAccess_execArrayStructAccessTest_Parms ArrayStructAccessTest_Parms;

	this->ProcessEvent ( pFnArrayStructAccessTest, &ArrayStructAccessTest_Parms, NULL );
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.SimpleStructAccess_CtorTest
// [0x00020002] 
// Parameters infos:

void UTest0022_StructMemberAccess::SimpleStructAccess_CtorTest ( )
{
	static UFunction* pFnSimpleStructAccess_CtorTest = NULL;

	if ( ! pFnSimpleStructAccess_CtorTest )
		pFnSimpleStructAccess_CtorTest = (UFunction*) UObject::GObjObjects()->Data[ 30890 ];

	UTest0022_StructMemberAccess_execSimpleStructAccess_CtorTest_Parms SimpleStructAccess_CtorTest_Parms;

	this->ProcessEvent ( pFnSimpleStructAccess_CtorTest, &SimpleStructAccess_CtorTest_Parms, NULL );
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.SimpleStructAccess_NonCtorTest
// [0x00020002] 
// Parameters infos:

void UTest0022_StructMemberAccess::SimpleStructAccess_NonCtorTest ( )
{
	static UFunction* pFnSimpleStructAccess_NonCtorTest = NULL;

	if ( ! pFnSimpleStructAccess_NonCtorTest )
		pFnSimpleStructAccess_NonCtorTest = (UFunction*) UObject::GObjObjects()->Data[ 30887 ];

	UTest0022_StructMemberAccess_execSimpleStructAccess_NonCtorTest_Parms SimpleStructAccess_NonCtorTest_Parms;

	this->ProcessEvent ( pFnSimpleStructAccess_NonCtorTest, &SimpleStructAccess_NonCtorTest_Parms, NULL );
};

// Function UnrealScriptTest.Test0022_StructMemberAccess.ReturnStructDelegate
// [0x00120000] 
// Parameters infos:
// struct FOuterStruct            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )

struct FOuterStruct UTest0022_StructMemberAccess::ReturnStructDelegate ( )
{
	static UFunction* pFnReturnStructDelegate = NULL;

	if ( ! pFnReturnStructDelegate )
		pFnReturnStructDelegate = (UFunction*) UObject::GObjObjects()->Data[ 30879 ];

	UTest0022_StructMemberAccess_execReturnStructDelegate_Parms ReturnStructDelegate_Parms;

	this->ProcessEvent ( pFnReturnStructDelegate, &ReturnStructDelegate_Parms, NULL );

	return ReturnStructDelegate_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0024_Commandlet.CommandletDelegateFunction
// [0x00020002] 
// Parameters infos:
// int                            Param1                         ( CPF_Parm )

void UTest0024_Commandlet::CommandletDelegateFunction ( int Param1 )
{
	static UFunction* pFnCommandletDelegateFunction = NULL;

	if ( ! pFnCommandletDelegateFunction )
		pFnCommandletDelegateFunction = (UFunction*) UObject::GObjObjects()->Data[ 30924 ];

	UTest0024_Commandlet_execCommandletDelegateFunction_Parms CommandletDelegateFunction_Parms;
	CommandletDelegateFunction_Parms.Param1 = Param1;

	this->ProcessEvent ( pFnCommandletDelegateFunction, &CommandletDelegateFunction_Parms, NULL );
};

// Function UnrealScriptTest.Test0024_Commandlet.Main
// [0x00020802] ( FUNC_Event )
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// struct FString                 Parms                          ( CPF_Parm | CPF_NeedCtorLink )

int UTest0024_Commandlet::eventMain ( struct FString Parms )
{
	static UFunction* pFnMain = NULL;

	if ( ! pFnMain )
		pFnMain = (UFunction*) UObject::GObjObjects()->Data[ 30919 ];

	UTest0024_Commandlet_eventMain_Parms Main_Parms;
	memcpy ( &Main_Parms.Parms, &Parms, 0xC );

	this->ProcessEvent ( pFnMain, &Main_Parms, NULL );

	return Main_Parms.ReturnValue;
};

// Function UnrealScriptTest.Test0024_Commandlet.CommandletDelegate
// [0x00120002] 
// Parameters infos:
// int                            I                              ( CPF_Parm )

void UTest0024_Commandlet::CommandletDelegate ( int I )
{
	static UFunction* pFnCommandletDelegate = NULL;

	if ( ! pFnCommandletDelegate )
		pFnCommandletDelegate = (UFunction*) UObject::GObjObjects()->Data[ 30916 ];

	UTest0024_Commandlet_execCommandletDelegate_Parms CommandletDelegate_Parms;
	CommandletDelegate_Parms.I = I;

	this->ProcessEvent ( pFnCommandletDelegate, &CommandletDelegate_Parms, NULL );
};

// Function UnrealScriptTest.Test0024_DelegateComparison.SomeOtherDelegate
// [0x00120000] 
// Parameters infos:
// unsigned long                  bFoo                           ( CPF_Parm )

void UTest0024_DelegateComparison::SomeOtherDelegate ( unsigned long bFoo )
{
	static UFunction* pFnSomeOtherDelegate = NULL;

	if ( ! pFnSomeOtherDelegate )
		pFnSomeOtherDelegate = (UFunction*) UObject::GObjObjects()->Data[ 30928 ];

	UTest0024_DelegateComparison_execSomeOtherDelegate_Parms SomeOtherDelegate_Parms;
	SomeOtherDelegate_Parms.bFoo = bFoo;

	this->ProcessEvent ( pFnSomeOtherDelegate, &SomeOtherDelegate_Parms, NULL );
};

// Function UnrealScriptTest.Test0024_DelegateComparison.DelegateExample
// [0x00020002] 
// Parameters infos:
// class UTest0024_Commandlet*    Owner                          ( CPF_Parm )

void UTest0024_DelegateComparison::DelegateExample ( class UTest0024_Commandlet* Owner )
{
	static UFunction* pFnDelegateExample = NULL;

	if ( ! pFnDelegateExample )
		pFnDelegateExample = (UFunction*) UObject::GObjObjects()->Data[ 30937 ];

	UTest0024_DelegateComparison_execDelegateExample_Parms DelegateExample_Parms;
	DelegateExample_Parms.Owner = Owner;

	this->ProcessEvent ( pFnDelegateExample, &DelegateExample_Parms, NULL );
};

// Function UnrealScriptTest.Test0024_DelegateComparison.DelegateFunction
// [0x00020002] 
// Parameters infos:
// int                            Param1                         ( CPF_Parm )

void UTest0024_DelegateComparison::DelegateFunction ( int Param1 )
{
	static UFunction* pFnDelegateFunction = NULL;

	if ( ! pFnDelegateFunction )
		pFnDelegateFunction = (UFunction*) UObject::GObjObjects()->Data[ 30935 ];

	UTest0024_DelegateComparison_execDelegateFunction_Parms DelegateFunction_Parms;
	DelegateFunction_Parms.Param1 = Param1;

	this->ProcessEvent ( pFnDelegateFunction, &DelegateFunction_Parms, NULL );
};

// Function UnrealScriptTest.Test0024_DelegateComparison.TestDelegateParam
// [0x00020002] 
// Parameters infos:
// struct FScriptDelegate         MyDelegate                     ( CPF_Parm | CPF_NeedCtorLink )

void UTest0024_DelegateComparison::TestDelegateParam ( struct FScriptDelegate MyDelegate )
{
	static UFunction* pFnTestDelegateParam = NULL;

	if ( ! pFnTestDelegateParam )
		pFnTestDelegateParam = (UFunction*) UObject::GObjObjects()->Data[ 30933 ];

	UTest0024_DelegateComparison_execTestDelegateParam_Parms TestDelegateParam_Parms;
	memcpy ( &TestDelegateParam_Parms.MyDelegate, &MyDelegate, 0xC );

	this->ProcessEvent ( pFnTestDelegateParam, &TestDelegateParam_Parms, NULL );
};

// Function UnrealScriptTest.Test0024_DelegateComparison.ComparisonDelegate
// [0x00120002] 
// Parameters infos:
// int                            Param1                         ( CPF_Parm )

void UTest0024_DelegateComparison::ComparisonDelegate ( int Param1 )
{
	static UFunction* pFnComparisonDelegate = NULL;

	if ( ! pFnComparisonDelegate )
		pFnComparisonDelegate = (UFunction*) UObject::GObjObjects()->Data[ 30930 ];

	UTest0024_DelegateComparison_execComparisonDelegate_Parms ComparisonDelegate_Parms;
	ComparisonDelegate_Parms.Param1 = Param1;

	this->ProcessEvent ( pFnComparisonDelegate, &ComparisonDelegate_Parms, NULL );
};

// Function UnrealScriptTest.Test0025_Commandlet.RunBoolTest
// [0x00020002] 
// Parameters infos:
// unsigned long                  bValueToApply                  ( CPF_Parm )
// unsigned long                  BoolVar                        ( CPF_Parm )

void UTest0025_Commandlet::RunBoolTest ( unsigned long bValueToApply, unsigned long BoolVar )
{
	static UFunction* pFnRunBoolTest = NULL;

	if ( ! pFnRunBoolTest )
		pFnRunBoolTest = (UFunction*) UObject::GObjObjects()->Data[ 30951 ];

	UTest0025_Commandlet_execRunBoolTest_Parms RunBoolTest_Parms;
	RunBoolTest_Parms.bValueToApply = bValueToApply;
	RunBoolTest_Parms.BoolVar = BoolVar;

	this->ProcessEvent ( pFnRunBoolTest, &RunBoolTest_Parms, NULL );
};

// Function UnrealScriptTest.Test0025_Commandlet.Main
// [0x00020802] ( FUNC_Event )
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// struct FString                 Params                         ( CPF_Parm | CPF_NeedCtorLink )

int UTest0025_Commandlet::eventMain ( struct FString Params )
{
	static UFunction* pFnMain = NULL;

	if ( ! pFnMain )
		pFnMain = (UFunction*) UObject::GObjObjects()->Data[ 30945 ];

	UTest0025_Commandlet_eventMain_Parms Main_Parms;
	memcpy ( &Main_Parms.Params, &Params, 0xC );

	this->ProcessEvent ( pFnMain, &Main_Parms, NULL );

	return Main_Parms.ReturnValue;
};

// Function UnrealScriptTest.TestCommandlet.Main
// [0x00020802] ( FUNC_Event )
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// struct FString                 Params                         ( CPF_Parm | CPF_NeedCtorLink )

int UTestCommandlet::eventMain ( struct FString Params )
{
	static UFunction* pFnMain = NULL;

	if ( ! pFnMain )
		pFnMain = (UFunction*) UObject::GObjObjects()->Data[ 30961 ];

	UTestCommandlet_eventMain_Parms Main_Parms;
	memcpy ( &Main_Parms.Params, &Params, 0xC );

	this->ProcessEvent ( pFnMain, &Main_Parms, NULL );

	return Main_Parms.ReturnValue;
};

// Function UnrealScriptTest.UDebuggerRegressionCommandlet.Main
// [0x00020802] ( FUNC_Event )
// Parameters infos:
// int                            ReturnValue                    ( CPF_Parm | CPF_OutParm | CPF_ReturnParm )
// struct FString                 Parms                          ( CPF_Parm | CPF_NeedCtorLink )

int UUDebuggerRegressionCommandlet::eventMain ( struct FString Parms )
{
	static UFunction* pFnMain = NULL;

	if ( ! pFnMain )
		pFnMain = (UFunction*) UObject::GObjObjects()->Data[ 30999 ];

	UUDebuggerRegressionCommandlet_eventMain_Parms Main_Parms;
	memcpy ( &Main_Parms.Parms, &Parms, 0xC );

	this->ProcessEvent ( pFnMain, &Main_Parms, NULL );

	return Main_Parms.ReturnValue;
};


#ifdef _MSC_VER
	#pragma pack ( pop )
#endif
