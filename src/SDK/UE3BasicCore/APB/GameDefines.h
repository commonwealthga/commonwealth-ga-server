/*
#############################################################################################
# Game: All Points Bulletin Reloaded														#
# Version: 1.6																				#
# ========================================================================================= #
# File: GameDefines.h																		#
#############################################################################################
*/

#ifdef _MSC_VER
	#pragma pack ( push, 0x4 )
#endif

/*
# ========================================================================================= #
# Defines																					
# ========================================================================================= #
*/

// Info
#define GAME_NAME				"All Points Bulletin Reloaded"
#define GAME_NAME_S				"APB"
#define GAME_VERSION			"1.6"

// Game
#define CLASS_ALIGN				0x4

// ProcessEvent Sig
#define ProcessEvent_Pattern	"\x83\xC0\x07\x83\xE0\xF8\xE8\x00\x00\x00\x00\x8B\xC4"
#define ProcessEvent_Mask		"xxxxxxx????xx"

// UE3 Tables Sigs
#define GObjects_Pattern		"\x8b\x00\x00\x00\x00\x00\x8b\x04\x00\x8b\x40\x00\x25\x00\x02\x00\x00"
#define GObjects_Mask			"x?????xx?xx?xxxxx"
#define GObjects_Offset			0x2

#define GNames_Pattern			"\x8b\x0d\x00\x00\x00\x00\x83\x3c\x81\x00\x74"
#define GNames_Mask				"xx????xxxxx"
#define GNames_Offset			0x2

// Function Flags ( incomplete )
#define FUNC_Final				0x00000001
#define FUNC_Latent				0x00000008				// ???	
#define FUNC_Simulated			0x00000100				// ???
#define FUNC_Exec				0x00000200
#define FUNC_Native				0x00000400
#define FUNC_Event				0x00000800

// Proprerty Flags ( c&p from UE3 source )
#define	CPF_Edit				0x0000000000000001		// Property is user-settable in the editor.
#define	CPF_Const				0x0000000000000002		// Actor's property always matches class's default actor property.
#define CPF_Input				0x0000000000000004		// Variable is writable by the input system.
#define CPF_ExportObject		0x0000000000000008		// Object can be exported with actor.
#define CPF_OptionalParm		0x0000000000000010		// Optional parameter (if CPF_Param is set).
#define CPF_Net					0x0000000000000020		// Property is relevant to network replication.
#define CPF_EditConstArray		0x0000000000000040		// Prevent adding/removing of items from dynamic a array in the editor.
#define CPF_Parm				0x0000000000000080		// Function/When call parameter.
#define CPF_OutParm				0x0000000000000100		// Value is copied out after function call.
#define CPF_SkipParm			0x0000000000000200		// Property is a short-circuitable evaluation function parm.
#define CPF_ReturnParm			0x0000000000000400		// Return value.
#define CPF_CoerceParm			0x0000000000000800		// Coerce args into this function parameter.
#define CPF_Native      		0x0000000000001000		// Property is native: C++ code is responsible for serializing it.
#define CPF_Transient   		0x0000000000002000		// Property is transient: shouldn't be saved, zero-filled at load time.
#define CPF_Config      		0x0000000000004000		// Property should be loaded/saved as permanent profile.
#define CPF_Localized   		0x0000000000008000		// Property should be loaded as localizable text.
#define CPF_Travel      		0x0000000000010000		// Property travels across levels/servers.
#define CPF_EditConst   		0x0000000000020000		// Property is uneditable in the editor.
#define CPF_GlobalConfig		0x0000000000040000		// Load config from base class, not subclass.
#define CPF_Component			0x0000000000080000		// Property containts component references.
#define CPF_NeedCtorLink		0x0000000000400000		// Fields need construction/destruction.
#define CPF_NoExport    		0x0000000000800000		// Property should not be exported to the native class header file.
#define CPF_NoClear				0x0000000002000000		// Hide clear (and browse) button.
#define CPF_EditInline			0x0000000004000000		// Edit this object reference inline.
#define CPF_EdFindable			0x0000000008000000		// References are set by clicking on actors in the editor viewports.
#define CPF_EditInlineUse		0x0000000010000000		// EditInline with Use button.
#define CPF_Deprecated  		0x0000000020000000		// Property is deprecated.  Read it from an archive, but don't save it.
#define CPF_EditInlineNotify	0x0000000040000000		// EditInline, notify outer object on editor change.
#define CPF_RepNotify			0x0000000100000000		// Notify actors when a property is replicated
#define CPF_Interp				0x0000000200000000		// interpolatable property for use with matinee
#define CPF_NonTransactional	0x0000000400000000		// Property isn't transacted

// Property SubClasses
#define CCP_UBYTE
#define CCP_UINT
#define CCP_UFLOAT
#define CCP_UBOOL
#define CCP_USTR
#define CCP_UNAME
#define CCP_UDELEGATE
#define CCP_UOBJECT
#define CCP_UCLASS
#define CCP_UINTERFACE
#define CCP_USTRUCT
#define CCP_UARRAY
#define CCP_UMAP

/*
# ========================================================================================= #
# Globals																					
# ========================================================================================= #
*/

unsigned long GObjects			= NULL;
unsigned long GNames			= NULL;

/*
# ========================================================================================= #
# Structs																					
# ========================================================================================= #
*/

template< class T > struct TArray
{
public:
	T* Data;
	int Count;
	int Max;

public:
	TArray()
	{
		Data = NULL;
		Count = Max = 0;
	};

public:
	int Num()
	{
		return this->Count;
	};

	T& operator() ( int i )
	{
		return this->Data[ i ];
	};

	const T& operator() ( int i ) const
	{ 
		return this->Data[ i ];
	};

	void Add ( T InputData )
	{
		Data = (T*) realloc ( Data, sizeof ( T ) * ( Count + 1 ) );
		Data[ Count++ ] = InputData;
		Max = Count;
	};

	void Clear()
	{
		free ( Data );
		Count = Max = 0;
	};
};

struct FNameEntry
{
	unsigned char	UnknownData00[ 0x10 ];
	wchar_t			Name[ 0x10 ];
};

struct FName
{
	int				Index;
	unsigned char	unknownData00[ 0x4 ];															

	FName() : Index ( 0 ) {};

	FName ( int i ) : Index ( i ) {};

	~FName() {};

	FName ( wchar_t* FindName )
	{
		static TArray< int > NameCache;

		for ( int i = 0; i < NameCache.Count; ++i )
		{
			if ( ! wcscmp ( this->Names()->Data[ NameCache ( i ) ]->Name, FindName ) )
			{
				Index = NameCache ( i );
				return;
			}
		}

		for ( int i = 0; i < this->Names()->Count; ++i )
		{
			if ( this->Names()->Data[ i ] )
			{
				if ( ! wcscmp ( this->Names()->Data[ i ]->Name, FindName ) )
				{
					NameCache.Add ( i );
					Index = i;
				}
			}
		}
	};

	static TArray< FNameEntry* >* Names()
	{		
		return (TArray< FNameEntry* >*) GNames;
	};

	wchar_t* GetName()
	{
		if ( Index < 0 || Index > this->Names()->Num() )
			return L"UnknownName";
		else
			return this->Names()->Data[ Index ]->Name;
	};

	bool operator == ( const FName& A ) const
	{
		return ( Index == A.Index );
	};
};

struct FString : public TArray< wchar_t >
{
	FString() {};

	FString ( wchar_t* Other )
	{
		this->Max = this->Count = *Other ? ( wcslen ( Other ) + 1 ) : 0;
		
		if ( this->Count )
			this->Data = Other;
	};

	~FString() {};

	FString operator = ( wchar_t* Other )
	{
		if ( this->Data != Other )
		{
			this->Max = this->Count = *Other ? ( wcslen ( Other ) + 1 ) : 0;

			if ( this->Count )
				this->Data = Other;
		}

		return *this;
	};
};

struct FScriptDelegate
{
	unsigned char UnknownData00[ 0xC ];
};

struct FPointer
{
	int Dummy;
};

struct FQWord
{
	int A;
	int B;
};

/*
# ========================================================================================= #
# Classes																					
# ========================================================================================= #
*/

// (0x0000 - 0x0040)
class UObject
{
public:
	struct FPointer			VfTableObject;							// 0x0000 (0x04)
	int						ObjectInternalInteger;					// 0x0004 (0x04)
	struct FQWord			ObjectFlags;                            // 0x0008 (0x08)
	struct FPointer			HashNext;                               // 0x0010 (0x04)
	struct FPointer			HashOuterNext;                          // 0x0014 (0x04)
	struct FPointer			StateFrame;                             // 0x0018 (0x04)
	class UObject*			Linker;                                 // 0x001C (0x04)
	struct FPointer			LinkerIndex;							// 0x0020 (0x04)
	int						NetIndex;                               // 0x0024 (0x04)
	int						nDynRefsIndex;							// 0x0028 (0x04)
	class UObject*			Outer;                                  // 0x002C (0x04)
	struct FName			Name;                                   // 0x0030 (0x08)
	class UClass*			Class;                                  // 0x0038 (0x04)
	class UObject*			ObjectArchetype;						// 0x003C (0x04)

private:
	static UClass* pClassPointer;

public:
	static TArray< UObject* >* GObjObjects();

	char* GetName();
	char* GetNameCPP();
	char* GetFullName();
	char* GetPackageName();
	UObject* GetPackageObj();
	
	template< class T > static T* FindObject ( char* ObjectFullName );
	template< class T > static unsigned int CountObject ( char* ObjectName );
	static UClass* FindClass ( char* ClassFullName );

	bool IsA ( UClass* pClass );

	static UClass* StaticClass()
	{
		if ( ! pClassPointer )
			pClassPointer = UObject::FindClass ( "Class Core.Object" );

		return pClassPointer;
	};
};

	// (0x0040 - 0x0048)
	class UField : public UObject
	{
	public:
		class UField*		SuperField;									// 0x0040 (0x04)
		class UField*		Next;										// 0x0044 (0x04)

	private:
		static UClass* pClassPointer;

	public:
		static UClass* StaticClass()
		{
			if ( ! pClassPointer )
				pClassPointer = UObject::FindClass ( "Class Core.Field" );

			return pClassPointer;
		};
	};

		// (0x0048 - 0x0054)
		class UEnum : public UField
		{
		public:
			TArray< FName >	Names;											// 0x0048 (0x0C)	

		private:
			static UClass* pClassPointer;

		public:
			static UClass* StaticClass()
			{
				if ( ! pClassPointer )
					pClassPointer = UObject::FindClass ( "Class Core.Enum" );

				return pClassPointer;
			};
		};

		// (0x0048 - 0x0054)
		class UConst : public UField
		{
		public:
			struct FString		Value;										// 0x0048 (0x0C)													

		private:
			static UClass* pClassPointer;

		public:
			static UClass* StaticClass()
			{
				if ( ! pClassPointer )
					pClassPointer = UObject::FindClass ( "Class Core.Const" );

				return pClassPointer;
			};
		};

		// (0x0048 - 0x0094)
		class UStruct : public UField
		{
		public:
			unsigned long			ScriptText;								// 0x0048 (0x04)
			unsigned long			CppText;								// 0x004C (0x04)		
			class UField*			Children;								// 0x0050 (0x04)
			unsigned long			PropertySize;							// 0x0054 (0x04)
			unsigned char			UnknownData00[ 0x8 ];					// 0x0058 (0x08) 
			TArray< unsigned char >	Script;									// 0x0060 (0x0C)
			unsigned char			UnknownData01[ 0x28 ];					// 0x006C (0x28)

		private:
			static UClass* pClassPointer;

		public:
			static UClass* StaticClass()
			{
				if ( ! pClassPointer )
					pClassPointer = UObject::FindClass ( "Class Core.Struct" );

				return pClassPointer;
			};
		};

			// (0x0094 - 0x00B0)
			class UScriptStruct : public UStruct
			{
			public:
				unsigned char		UnknownData00[ 0x1C ];						// 0x0094 (0x1C)

			private:
				static UClass* pClassPointer;

			public:
				static UClass* StaticClass()
				{
					if ( ! pClassPointer )
						pClassPointer = UObject::FindClass ( "Class Core.ScriptStruct" );

					return pClassPointer;
				};
			};

			// (0x0094 - 0x00B4)
			class UFunction : public UStruct
			{
			public:
				unsigned long		FunctionFlags;								// 0x0094 (0x04)
				unsigned short		iNative;									// 0x0098 (0x02)
				unsigned short		RepOffset;									// 0x009A (0x02)
				unsigned char		OperPrecedence;								// 0x009C (0x01)
				struct FName		FriendlyName;								// 0x00A0 (0x08)
				unsigned char		NumParms;									// 0x00A8 (0x01)
				unsigned short		ParmsSize;									// 0x00AC (0x02)
				unsigned short		ReturnValueOffset;							// 0x00AE (0x02)
				void*				Func;										// 0x00B0 (0x04)

			private:
				static UClass* pClassPointer;

			public:
				static UClass* StaticClass()
				{
					if ( ! pClassPointer )
						pClassPointer = UObject::FindClass ( "Class Core.Function" );

					return pClassPointer;
				};
			};

			// (0x0094 - 0x00E8)
			class UState : public UStruct
			{
			public:
				unsigned char			UnknownData00[ 0x54 ];						// 0x0094 (0x54)

			private:
				static UClass* pClassPointer;

			public:
				static UClass* StaticClass()
				{
					if ( ! pClassPointer )
						pClassPointer = UObject::FindClass ( "Class Core.State" );

					return pClassPointer;
				};
			};

				// (0x00E8 - 0x01F8)
				class UClass : public UState
				{
				public:
					unsigned char			UnknownData00[ 0x110 ];					// 0x00E8 (0x110)

				private:
					static UClass* pClassPointer;

				public:
					static UClass* StaticClass()
					{
						if ( ! pClassPointer )
							pClassPointer = UObject::FindClass ( "Class Core.Class" );

						return pClassPointer;
					};
				};

		// (0x0048 - 0x0088)
		class UProperty : public UField 
		{
		public:
			unsigned long		ArrayDim;									// 0x0048 (0x04)					
			unsigned long		ElementSize;								// 0x004C (0x04)
			FQWord				PropertyFlags;								// 0x0050 (0x08)
			unsigned char		UnknownData00[ 0x10 ];						// 0x0058 (0x10)
			unsigned long		Offset;										// 0x0068 (0x04)
			unsigned char		UnknownData01[ 0x1C ];						// 0x006C (0x1C)

		private:
			static UClass* pClassPointer;

		public:
			static UClass* StaticClass()
			{
				if ( ! pClassPointer )
					pClassPointer = UObject::FindClass ( "Class Core.Property" );

				return pClassPointer;
			};
		};

			// (0x0088 - 0x008C)
			class UByteProperty : public UProperty 
			{
			public:
				class UEnum*		Enum;										// 0x0088 (0x04)

			private:
				static UClass* pClassPointer;

			public:
				static UClass* StaticClass()
				{
					if ( ! pClassPointer )
						pClassPointer = UObject::FindClass ( "Class Core.ByteProperty" );

					return pClassPointer;
				};
			};

			// (0x0088 - 0x0088)
			class UIntProperty : public UProperty 
			{
			public:

			private:
				static UClass* pClassPointer;

			public:
				static UClass* StaticClass()
				{
					if ( ! pClassPointer )
						pClassPointer = UObject::FindClass ( "Class Core.IntProperty" );

					return pClassPointer;
				};
			};

			// (0x0088 - 0x0088)
			class UFloatProperty : public UProperty 
			{
			public:

			private:
				static UClass* pClassPointer;

			public:
				static UClass* StaticClass()
				{
					if ( ! pClassPointer )
						pClassPointer = UObject::FindClass ( "Class Core.FloatProperty" );

					return pClassPointer;
				};
			};

			// (0x0088 - 0x008C)
			class UBoolProperty : public UProperty 
			{
			public:
				unsigned long		BitMask;									// 0x0088 (0x04)			

			private:
				static UClass* pClassPointer;

			public:
				static UClass* StaticClass()
				{
					if ( ! pClassPointer )
						pClassPointer = UObject::FindClass ( "Class Core.BoolProperty" );

					return pClassPointer;
				};
			};

			// (0x0088 - 0x0088)
			class UStrProperty : public UProperty 
			{
			public:

			private:
				static UClass* pClassPointer;

			public:
				static UClass* StaticClass()
				{
					if ( ! pClassPointer )
						pClassPointer = UObject::FindClass ( "Class Core.StrProperty" );

					return pClassPointer;
				};
			};

			// (0x0088 - 0x0088)
			class UNameProperty : public UProperty 
			{
			public:

			private:
				static UClass* pClassPointer;

			public:
				static UClass* StaticClass()
				{
					if ( ! pClassPointer )
						pClassPointer = UObject::FindClass ( "Class Core.NameProperty" );

					return pClassPointer;
				};
			};

			// (0x0088 - 0x0090)
			class UDelegateProperty : public UProperty 
			{
			public:
				unsigned char			UnknownData00[ 0x8 ];						// 0x0088 (0x08)

			private:
				static UClass* pClassPointer;

			public:
				static UClass* StaticClass()
				{
					if ( ! pClassPointer )
						pClassPointer = UObject::FindClass ( "Class Core.DelegateProperty" );

					return pClassPointer;
				};
			};

			// (0x0088 - 0x008C)
			class UObjectProperty : public UProperty 
			{
			public:
				class UClass*		PropertyClass;								// 0x0088 (0x04)

			private:
				static UClass* pClassPointer;

			public:
				static UClass* StaticClass()
				{
					if ( ! pClassPointer )
						pClassPointer = UObject::FindClass ( "Class Core.ObjectProperty" );

					return pClassPointer;
				};
			};

				// (0x008C - 0x0090)
				class UClassProperty : public UObjectProperty 
				{
				public:
					class UClass*		MetaClass;									// 0x008C (0x04)

				private:
					static UClass* pClassPointer;

				public:
					static UClass* StaticClass()
					{
						if ( ! pClassPointer )
							pClassPointer = UObject::FindClass ( "Class Core.ClassProperty" );

						return pClassPointer;
					};
				};

			// (0x0088 - 0x008C)
			class UInterfaceProperty : public UProperty
			{
			public:
				class UClass*		InterfaceClass;								// 0x0088 (0x04) 

			private:
				static UClass* pClassPointer;

			public:
				static UClass* StaticClass()
				{
					if ( ! pClassPointer )
						pClassPointer = UObject::FindClass ( "Class Core.InterfaceProperty" );

					return pClassPointer;
				};
			};

			// (0x0088 - 0x008C)
			class UStructProperty : public UProperty 
			{
			public:
				class UStruct*		Struct;										// 0x0088 (0x04)

			private:
				static UClass* pClassPointer;

			public:
				static UClass* StaticClass()
				{
					if ( ! pClassPointer )
						pClassPointer = UObject::FindClass ( "Class Core.StructProperty" );

					return pClassPointer;
				};
			};

			// (0x0088 - 0x008C)
			class UArrayProperty : public UProperty 
			{
			public:
				class UProperty*	Inner;										// 0x0088 (0x04)

			private:
				static UClass* pClassPointer;

			public:
				static UClass* StaticClass()
				{
					if ( ! pClassPointer )
						pClassPointer = UObject::FindClass ( "Class Core.ArrayProperty" );

					return pClassPointer;
				};
			};

			// (0x0088 - 0x0090)
			class UMapProperty : public UProperty 
			{
			public:
				class UProperty*	Key;										// 0x0088 (0x04)
				class UProperty*	Value;										// 0x008C (0x04)

			private:
				static UClass* pClassPointer;

			public:
				static UClass* StaticClass()
				{
					if ( ! pClassPointer )
						pClassPointer = UObject::FindClass ( "Class Core.MapProperty" );

					return pClassPointer;
				};
			};

/*
# ========================================================================================= #
# Init Core Classes Pointers																
# ========================================================================================= #
*/

UClass* UObject							::pClassPointer = NULL;
UClass*		UField						::pClassPointer = NULL;
UClass*			UEnum					::pClassPointer = NULL;
UClass*			UConst					::pClassPointer = NULL;
UClass*			UStruct					::pClassPointer = NULL;
UClass*				UScriptStruct		::pClassPointer = NULL;
UClass*				UFunction			::pClassPointer = NULL;
UClass*				UState				::pClassPointer = NULL;
UClass*					UClass			::pClassPointer = NULL;
UClass*			UProperty				::pClassPointer = NULL;
UClass*			UByteProperty			::pClassPointer = NULL;
UClass*			UIntProperty			::pClassPointer = NULL;
UClass*			UFloatProperty			::pClassPointer = NULL;
UClass*			UBoolProperty			::pClassPointer = NULL;
UClass*			UStrProperty			::pClassPointer = NULL;
UClass*			UNameProperty			::pClassPointer = NULL;
UClass*			UDelegateProperty		::pClassPointer = NULL;
UClass*			UObjectProperty			::pClassPointer = NULL;
UClass*				UClassProperty		::pClassPointer = NULL;
UClass*			UInterfaceProperty		::pClassPointer = NULL;
UClass*			UStructProperty			::pClassPointer = NULL;
UClass*			UArrayProperty			::pClassPointer = NULL;
UClass*			UMapProperty			::pClassPointer = NULL;

/*
# ========================================================================================= #
#
# ========================================================================================= #
*/

#ifdef _MSC_VER
	#pragma pack ( pop )
#endif