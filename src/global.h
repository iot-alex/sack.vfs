#include <node.h>
//#include <nan.h>
#include <node_object_wrap.h>
#include <v8.h>
#include <uv.h>

#define V8_AT_LEAST(major, minor) (V8_MAJOR_VERSION > major || (V8_MAJOR_VERSION == major && V8_MINOR_VERSION >= minor))
//#include <nan.h>

#ifdef SACK_CORE
#include <stdhdrs.h>
#include <network.h>
//#include <idle.h>
#include <pssql.h>
#include <sack_vfs.h>
#include <salty_generator.h>
#include <filesys.h>
#include <deadstart.h>
#include <procreg.h>
#include <translation.h>
#include <json_emitter.h>
#include <jsox_parser.h>
#include <vesl_emitter.h>
#include <sqlgetoption.h>
#include <sackcomm.h>
#include <html5.websocket.h>
#include <html5.websocket.client.h>
#include <http.h>
#else
#include "sack/sack.h"
#endif

#undef New

//#include <openssl/ssl.h>
#include <openssl/safestack.h>  // STACK_OF
#include <openssl/tls1.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>
//#include <openssl/>

#ifdef INCLUDE_GUI
#include "gui/gui_global.h"

#endif


#if NODE_MAJOR_VERSION >= 10
#  define USE_ISOLATE(i)   (i),
#  define USE_ISOLATE_VOID(i)   (i)
#else
#  define USE_ISOLATE(i)
#  define USE_ISOLATE_VOID(i)
#endif


using namespace v8;

#include "task_module.h"

//fileObject->DefineOwnProperty( isolate->GetCurrentContext(), String::NewFromUtf8( isolate, "SeekSet" ), Integer::New( isolate, SEEK_SET ), ReadOnlyProperty );

#define SET_READONLY( object, name, data ) (object)->DefineOwnProperty( isolate->GetCurrentContext(), String::NewFromUtf8(isolate, name), data, ReadOnlyProperty )
#define SET_READONLY_METHOD( object, name, method ) (object)->DefineOwnProperty( isolate->GetCurrentContext(), String::NewFromUtf8(isolate, name), v8::Function::New(isolate, method ), ReadOnlyProperty )

void InitJSOX( Isolate *isolate, Handle<Object> exports );
void InitJSON( Isolate *isolate, Handle<Object> exports );
void InitSRG( Isolate *isolate, Handle<Object> exports );
void InitWebSocket( Isolate *isolate, Handle<Object> exports );
void InitUDPSocket( Isolate *isolate, Handle<Object> exports );
void InitTask( Isolate *isolate, Handle<Object> exports );
void KeyHidObjectInit( Isolate *isolate, Handle<Object> exports );

#define ReadOnlyProperty (PropertyAttribute)((int)PropertyAttribute::ReadOnly | PropertyAttribute::DontDelete)



class VolumeObject : public node::ObjectWrap {
public:
	struct volume *vol;
	bool volNative;
	char *mountName;
	char *fileName;
	struct file_system_interface *fsInt;
	struct file_system_mounted_interface* fsMount;
	static v8::Persistent<v8::Function> constructor;

public:

	static void Init( Handle<Object> exports );
	VolumeObject( const char *mount, const char *filename, uintptr_t version, const char *key, const char *key2 );

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void getDirectory( const v8::FunctionCallbackInfo<Value>& args );
	static void fileRead( const v8::FunctionCallbackInfo<Value>& args );
	static void fileReadString( const v8::FunctionCallbackInfo<Value>& args );
	static void fileReadMemory( const v8::FunctionCallbackInfo<Value>& args );
	static void fileReadJSON( const v8::FunctionCallbackInfo<Value>& args );
	static void fileReadJSOX( const v8::FunctionCallbackInfo<Value>& args );
	static void fileWrite( const v8::FunctionCallbackInfo<Value>& args );
	static void fileExists( const v8::FunctionCallbackInfo<Value>& args );
	static void openVolDb( const v8::FunctionCallbackInfo<Value>& args );
	static void fileVolDelete( const v8::FunctionCallbackInfo<Value>& args );
	static void makeDirectory( const v8::FunctionCallbackInfo<Value>& args );
	static void mkdir( const v8::FunctionCallbackInfo<Value>& args );
	static void volRekey( const v8::FunctionCallbackInfo<Value>& args );
	static void renameFile( const v8::FunctionCallbackInfo<Value>& args );
	static void volDecrypt( const v8::FunctionCallbackInfo<Value>& args );

	~VolumeObject();
};


class ObjectStorageObject : public node::ObjectWrap {
public:
	struct volume *vol;
	//bool volNative;
	char *mountName;
	char *fileName;
	//struct file_system_interface *fsInt;
	//struct file_system_mounted_interface* fsMount;
	static v8::Persistent<v8::Function> constructor;

public:

	static void Init( Isolate *isolate, Handle<Object> exports );
	ObjectStorageObject( const char *mount, const char *filename, uintptr_t version, const char *key, const char *key2 );

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	// get object pass object ID
	static void getObject( const v8::FunctionCallbackInfo<Value>& args );

	// get object and all recursive objects associated from here (for 1 level?)
	static void mapObject( const v8::FunctionCallbackInfo<Value>& args );

	// pass object, result with object ID.
	static void putObject( const v8::FunctionCallbackInfo<Value>& args );

	// pass object ID, get back a ObjectStorageFileObject ( support seek/read/write? )
	static void openObject( const v8::FunctionCallbackInfo<Value>& args );

	// utility to remove the key so it can be diagnosed.
	static void volDecrypt( const v8::FunctionCallbackInfo<Value>& args );

	~ObjectStorageObject();
};


class ThreadObject : public node::ObjectWrap {
public:
	static v8::Persistent<v8::Function> constructor;
   	static Persistent<Function> idleProc;
public:

	static void Init( Handle<Object> exports );
	ThreadObject();

	static void New( const v8::FunctionCallbackInfo<Value>& args );

	static void relinquish( const v8::FunctionCallbackInfo<Value>& args );
	static void wake( const v8::FunctionCallbackInfo<Value>& args );

	~ThreadObject();
};


class FileObject : public node::ObjectWrap {
	VolumeObject *vol;
	struct sack_vfs_file *file;
	FILE *cfile;
	//Local<Object> volume;
	PLIST buffers; // because these are passed as physical data buffers, don't release them willy-nilly
	char* buf;
	size_t size;
	Persistent<Object> volume;
public:
	static v8::Persistent<v8::Function> constructor;
	static Persistent<FunctionTemplate> tpl;
	static void Init(  );

	static void openFile( const v8::FunctionCallbackInfo<Value>& args );
	static void readFile( const v8::FunctionCallbackInfo<Value>& args );
	static void readLine( const v8::FunctionCallbackInfo<Value>& args );
	static void writeFile( const v8::FunctionCallbackInfo<Value>& args );
	static void writeLine( const v8::FunctionCallbackInfo<Value>& args );
	static void seekFile( const v8::FunctionCallbackInfo<Value>& args );
	static void tellFile( const v8::FunctionCallbackInfo<Value>& args );
	static void truncateFile( const v8::FunctionCallbackInfo<Value>& args );

	//static void readFile( const v8::FunctionCallbackInfo<Value>& args );

	static void Emitter( const v8::FunctionCallbackInfo<Value>& args );

	FileObject( VolumeObject* vol, const char *filename, Isolate*, Local<Object> o );
	~FileObject();
};

struct SqlObjectUserFunction {
	class SqlObject *sql;
	Persistent<Function> cb;
	Persistent<Function> cb2;
	Isolate *isolate;
};

class SqlStmtObject : public node::ObjectWrap {
public:
	static v8::Persistent<v8::Function> constructor;
	class SqlObject *sql;
	PDATALIST values;
	SqlStmtObject() {
		values = NULL;
	}
	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void Set( const v8::FunctionCallbackInfo<Value>& args );
};

class SqlObject : public node::ObjectWrap {
public:
	PODBC odbc;
	int optionInitialized;
	static v8::Persistent<v8::Function> constructor;
	int columns;
	CTEXTSTR *result;
	size_t *resultLens;
	CTEXTSTR *fields;
	v8::Persistent<v8::Function> onCorruption;
	Persistent<Object> _this;
	//Persistent<Object> volume;
public:
	PTHREAD thread;
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLIST userFunctions;
	PLINKQUEUE messages;

	static void Init( Handle<Object> exports );
	SqlObject( const char *dsn );

	static void New( const v8::FunctionCallbackInfo<Value>& args );
	static void query( const v8::FunctionCallbackInfo<Value>& args );
	static void escape( const v8::FunctionCallbackInfo<Value>& args );
	static void unescape( const v8::FunctionCallbackInfo<Value>& args );
	static void option( const v8::FunctionCallbackInfo<Value>& args );
	static void setOption( const v8::FunctionCallbackInfo<Value>& args );
	static void optionInternal( const v8::FunctionCallbackInfo<Value>& args );
	static void setOptionInternal( const v8::FunctionCallbackInfo<Value>& args );
	static void makeTable( const v8::FunctionCallbackInfo<Value>& args );
	static void closeDb( const v8::FunctionCallbackInfo<Value>& args );
	static void commit( const v8::FunctionCallbackInfo<Value>& args );
	static void transact( const v8::FunctionCallbackInfo<Value>& args );
	static void autoTransact( const v8::FunctionCallbackInfo<Value>& args );
	static void userFunction( const v8::FunctionCallbackInfo<Value>& args );
	static void userProcedure( const v8::FunctionCallbackInfo<Value>& args );
	static void aggregateFunction( const v8::FunctionCallbackInfo<Value>& args );
	static void setOnCorruption( const v8::FunctionCallbackInfo<Value>& args );

	static void enumOptionNodes( const v8::FunctionCallbackInfo<Value>& args );
	static void enumOptionNodesInternal( const v8::FunctionCallbackInfo<Value>& args );
	static void findOptionNode( const v8::FunctionCallbackInfo<Value>& args );
	static void getOptionNode( const v8::FunctionCallbackInfo<Value>& args );
	static void error( const v8::FunctionCallbackInfo<Value>& args );

	static void getLogging( const v8::FunctionCallbackInfo<Value>& args );
	static void setLogging( const v8::FunctionCallbackInfo<Value>& args );


	static void doWrap( SqlObject *sql, Local<Object> o ); 

	~SqlObject();
};


class OptionTreeObject : public node::ObjectWrap {
public:
	POPTION_TREE_NODE node;
	PODBC odbc;
	static v8::Persistent<v8::Function> constructor;
	
public:

	static void Init( );
	OptionTreeObject(  );

	static void New( const v8::FunctionCallbackInfo<Value>& args );

	static void enumOptionNodes( const v8::FunctionCallbackInfo<Value>& args );
	static void findOptionNode( const v8::FunctionCallbackInfo<Value>& args );
	static void getOptionNode( const v8::FunctionCallbackInfo<Value>& args );
	static void writeOptionNode( v8::Local<v8::String> field,
		v8::Local<v8::Value> val,
		const PropertyCallbackInfo<void>&info );
	static void readOptionNode( v8::Local<v8::String> field,
		const PropertyCallbackInfo<v8::Value>& info );

	~OptionTreeObject();
};




class ComObject : public node::ObjectWrap {
public:
	int handle;
	char *name;
	static Persistent<Function> constructor;

	Persistent<Function, CopyablePersistentTraits<Function>> *readCallback; //
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLINKQUEUE readQueue;
	
public:

	static void Init( Handle<Object> exports );
	ComObject( char *name );

private:
	Persistent<Object> jsObject;
	static void New( const v8::FunctionCallbackInfo<Value>& args );

	static void onRead( const v8::FunctionCallbackInfo<Value>& args );
	static void writeCom( const v8::FunctionCallbackInfo<Value>& args );
	static void closeCom( const v8::FunctionCallbackInfo<Value>& args );
	~ComObject();
};


class RegObject : public node::ObjectWrap {
public:
	static void Init( Handle<Object> exports );

private:
	static void getRegItem( const v8::FunctionCallbackInfo<Value>& args );
	static void setRegItem( const v8::FunctionCallbackInfo<Value>& args );
};


class WebSockClientObject : public node::ObjectWrap {
public:
	static Persistent<Function> constructor;

	Persistent<Function, CopyablePersistentTraits<Function>> closeCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> errorCallback; //
	Persistent<Function, CopyablePersistentTraits<Function>> readCallback; //
	uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLINKQUEUE readQueue;
	
public:

	static void Init( Handle<Object> exports );
	WebSockClientObject();

	static void New( const v8::FunctionCallbackInfo<Value>& args );

	static void open( const v8::FunctionCallbackInfo<Value>& args );
	static void on( const v8::FunctionCallbackInfo<Value>& args );
	static void send( const v8::FunctionCallbackInfo<Value>& args );
	static void close( const v8::FunctionCallbackInfo<Value>& args );


	~WebSockClientObject();
};

class TLSObject : public node::ObjectWrap {

public:
	static v8::Persistent<v8::Function> constructor;

public:

	static void Init( Isolate *isolate, Handle<Object> exports );
	TLSObject( );

	static void New( const v8::FunctionCallbackInfo<Value>& args );

	static void seed( const v8::FunctionCallbackInfo<Value>& args );
	static void genKey( const v8::FunctionCallbackInfo<Value>& args );
	static void genCert( const v8::FunctionCallbackInfo<Value>& args );
	static void genReq( const v8::FunctionCallbackInfo<Value>& args );
	static void signReq( const v8::FunctionCallbackInfo<Value>& args );
	static void pubKey( const v8::FunctionCallbackInfo<Value>& args );
	static void validate( const v8::FunctionCallbackInfo<Value>& args );
	static void expiration( const v8::FunctionCallbackInfo<Value>& args );
	static void certToString( const v8::FunctionCallbackInfo<Value>& args );


	~TLSObject();
};

struct reviver_data {
	Isolate *isolate;
	Local<Context> context;
	LOGICAL revive;
	int index;
	Handle<Value> value;
	Handle<Object> _this;
	Handle<Function> reviver;
	Handle<Object> rootObject;
};

Local<Value> convertMessageToJS( PDATALIST msg_data, struct reviver_data *reviver );
Local<Value> convertMessageToJS2( PDATALIST msg_data, struct reviver_data *reviver );

struct arrayBufferHolder {
	void *buffer;
	Persistent<Object> o;
	Persistent<String> s;
	Persistent<ArrayBuffer> ab;
};
typedef struct arrayBufferHolder ARRAY_BUFFER_HOLDER, *PARRAY_BUFFER_HOLDER;
#define MAXARRAY_BUFFER_HOLDERSPERSET 128
DeclareSet( ARRAY_BUFFER_HOLDER );

void releaseBuffer( const WeakCallbackInfo<ARRAY_BUFFER_HOLDER> &info );
Local<String> localString( Isolate *isolate, const char *data, int len );


void InitFS( const v8::FunctionCallbackInfo<Value>& args );
void ConfigScriptInit( Handle<Object> exports );

#ifndef VFS_MAIN_SOURCE
extern
#endif
struct vfs_global_data {
	ARRAY_BUFFER_HOLDERSET *holders;
} vfs_global_data;

#define GetHolder() GetFromSet( ARRAY_BUFFER_HOLDER, &vfs_global_data.holders )
#define DropHolder(h) DeleteFromSet( ARRAY_BUFFER_HOLDER, vfs_global_data.holders, h )

