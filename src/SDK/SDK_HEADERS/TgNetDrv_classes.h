/*
#############################################################################################
# Global Agenda (1.5.1.5) SDK
# Generated with TheFeckless UE3 SDK Generator v1.4_Beta-Rev.51
# ========================================================================================= #
# File: TgNetDrv_classes.h
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


/*
# ========================================================================================= #
# Enums
# ========================================================================================= #
*/


/*
# ========================================================================================= #
# Classes
# ========================================================================================= #
*/

// Class TgNetDrv.UdpNetDriver
// 0x05F8 (0x0740 - 0x0148)
class UUdpNetDriver : public UNetDriver
{
public:
	unsigned char                                      UnknownData00[ 0x5F4 ];                           		// 0x0148 (0x05F8) MISSED OFFSET

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1586 ];

		return pClassPointer;
	};

};


// Class TgNetDrv.MarshalChannel
// 0x0068 (0x00D0 - 0x0068)
class UMarshalChannel : public UChannel
{
public:
	unsigned char                                      UnknownData00[ 0x68 ];                            		// 0x0068 (0x0068) MISSED OFFSET

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1587 ];

		return pClassPointer;
	};

};


// Class TgNetDrv.ClientConnection
// 0x0004 (0x4FB8 - 0x4FB4)
class UClientConnection : public UNetConnection
{
public:
	unsigned char                                      UnknownData00[ 0x4 ];                             		// 0x4FB4 (0x0004) MISSED OFFSET

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 84387 ];

		return pClassPointer;
	};

};



#ifdef _MSC_VER
	#pragma pack ( pop )
#endif
