/*
#############################################################################################
# Global Agenda (1.5.1.5) SDK
# Generated with TheFeckless UE3 SDK Generator v1.4_Beta-Rev.51
# ========================================================================================= #
# File: ALAudio_classes.h
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

// Class ALAudio.ALAudioDevice
// 0x007C (0x0228 - 0x01AC)
class UALAudioDevice : public UAudioDevice
{
public:
	struct FString                                     DeviceName;                                       		// 0x01AC (0x000C) [0x0000000000404000]              ( CPF_Config | CPF_NeedCtorLink )
	unsigned char                                      UnknownData00[ 0x68 ];                            		// 0x01B8 (0x0068) MISSED OFFSET
	float                                              MinOggVorbisDurationEditor;                       		// 0x0220 (0x0004) [0x0000000000004000]              ( CPF_Config )
	float                                              MinOggVorbisDurationGame;                         		// 0x0224 (0x0004) [0x0000000000004000]              ( CPF_Config )

private:
	inline static UClass* pClassPointer = nullptr;

public:
	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = (UClass*) UObject::GObjObjects()->Data[ 1728 ];

		return pClassPointer;
	};

};



#ifdef _MSC_VER
	#pragma pack ( pop )
#endif
