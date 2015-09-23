#ifndef RECOGNIZER_H
#define RECOGNIZER_H

#include <uv.h>
#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <pocketsphinx.h>
#include <sphinxbase/err.h>
#include <sphinxbase/jsgf.h>

#include <vector>

class Recognizer : public node::ObjectWrap
{
public:
	static void Init(v8::Handle<v8::Object> exports);

private:
	explicit Recognizer();
	~Recognizer();

	static void New(const v8::FunctionCallbackInfo<v8::Value>&);
	static void Free(const v8::FunctionCallbackInfo<v8::Value>&);
	static void Reconfig(const v8::FunctionCallbackInfo<v8::Value>&);

	static void SilenceDetection(const v8::FunctionCallbackInfo<v8::Value>&);

	static void On(const v8::FunctionCallbackInfo<v8::Value>&);
	static void Off(const v8::FunctionCallbackInfo<v8::Value>&);

	static void Start(const v8::FunctionCallbackInfo<v8::Value>&);
	static void Stop(const v8::FunctionCallbackInfo<v8::Value>&);
	static void Restart(const v8::FunctionCallbackInfo<v8::Value>&);

	static void Write(const v8::FunctionCallbackInfo<v8::Value>&);
	static void WriteSync(const v8::FunctionCallbackInfo<v8::Value>&);

	static void LookupWords(const v8::FunctionCallbackInfo<v8::Value>&);
	static void AddWords(const v8::FunctionCallbackInfo<v8::Value>&);

	static void AddKeyphraseSearch(const v8::FunctionCallbackInfo<v8::Value>&);
	static void AddKeywordsSearch(const v8::FunctionCallbackInfo<v8::Value>&);
	static void AddGrammarSearch(const v8::FunctionCallbackInfo<v8::Value>&);
	static void AddNgramSearch(const v8::FunctionCallbackInfo<v8::Value>&);

	static void GetSearch(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>&);
	static void SetSearch(v8::Local<v8::String>, v8::Local<v8::Value>, const v8::PropertyCallbackInfo<void>&);

	static void FromFloat(const v8::FunctionCallbackInfo<v8::Value>&);

	static v8::Persistent<v8::Function> constructor;
	static void AsyncWorker(uv_work_t* request);
	static void AsyncAfter(uv_work_t* request);

	static v8::Local<v8::Value> Default(v8::Local<v8::Value> value, v8::Local<v8::Value> fallback);
	static cmd_ln_t* BuildConfig(v8::Handle<v8::Object> options);

	static void Error(Recognizer* instance, v8::Isolate* isolate, const v8::Handle<v8::String> msg);
	static void TypeError(Recognizer* instance, v8::Isolate* isolate, const v8::Handle<v8::String> msg);

	ps_decoder_t* ps;

	v8::Persistent<v8::Function> hypCallback;
	v8::Persistent<v8::Function> hypFinalCallback;
	v8::Persistent<v8::Function> startCallback;
	v8::Persistent<v8::Function> stopCallback;
	v8::Persistent<v8::Function> speechDetectedCallback;
	v8::Persistent<v8::Function> silenceDetectedCallback;
	v8::Persistent<v8::Function> errorCallback;

	bool destructed;
	bool processing;

	// Silence detection
	bool silenceDetection;
	bool speechDetected;

	//bool isFirstDecoding;
};

typedef struct AsyncData {
  Recognizer* instance;
  v8::Handle<v8::Value> exception;
  bool hasException;
  int16* data;
  size_t length;
  int32 score;
  const char* hyp;
} AsyncData;

#endif
