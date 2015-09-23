#include <node.h>
#include <iostream>
#include <node_buffer.h>
#include "Recognizer.h"

using namespace v8;
using namespace std;

Recognizer::Recognizer() {

}

Recognizer::~Recognizer() {
	if(destructed == false) {
		processing = false;
		ps_free(ps);
	}
	destructed = true;
}

Persistent<Function> Recognizer::constructor;

void Recognizer::Init(Handle<Object> exports) {
	Isolate* isolate = Isolate::GetCurrent();

	Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
	tpl->SetClassName(String::NewFromUtf8(isolate,"Recognizer"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	tpl->Set(String::NewFromUtf8(isolate, "modelDirectory"), String::NewFromUtf8(isolate, MODELDIR));
	tpl->PrototypeTemplate()->SetAccessor(String::NewFromUtf8(isolate, "search"), GetSearch, SetSearch);

	NODE_SET_PROTOTYPE_METHOD(tpl, "free", Free);
	NODE_SET_PROTOTYPE_METHOD(tpl, "reconfig", Reconfig);
	NODE_SET_PROTOTYPE_METHOD(tpl, "silenceDetection", SilenceDetection);

	NODE_SET_PROTOTYPE_METHOD(tpl, "on", On);
	NODE_SET_PROTOTYPE_METHOD(tpl, "off", Off);

	NODE_SET_PROTOTYPE_METHOD(tpl, "start", Start);
	NODE_SET_PROTOTYPE_METHOD(tpl, "stop", Stop);
	NODE_SET_PROTOTYPE_METHOD(tpl, "restart", Restart);
	NODE_SET_PROTOTYPE_METHOD(tpl, "addKeyphraseSearch", AddKeyphraseSearch);
	NODE_SET_PROTOTYPE_METHOD(tpl, "addKeywordsSearch", AddKeywordsSearch);
	NODE_SET_PROTOTYPE_METHOD(tpl, "addGrammarSearch", AddGrammarSearch);
	NODE_SET_PROTOTYPE_METHOD(tpl, "addNgramSearch", AddNgramSearch);

	NODE_SET_PROTOTYPE_METHOD(tpl, "write", Write);
	NODE_SET_PROTOTYPE_METHOD(tpl, "writeSync", WriteSync);

	NODE_SET_PROTOTYPE_METHOD(tpl, "lookupWords", LookupWords);
	NODE_SET_PROTOTYPE_METHOD(tpl, "addWords", AddWords);

	// @deprecated fromFloat should be called directly like PocketSphinx.fromFloat(buffer)
	NODE_SET_PROTOTYPE_METHOD(tpl, "fromFloat", FromFloat);
	
	constructor.Reset(isolate, tpl->GetFunction());
	exports->Set(String::NewFromUtf8(isolate, "Recognizer"), tpl->GetFunction());

	NODE_SET_METHOD(exports, "fromFloat", FromFloat);
}

void Recognizer::New(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);

	if(!args.IsConstructCall()) {
		const int argc = 2;
		Local<Value> argv[argc] = { args[0], args[1] };
		Local<Function> cons = Local<Function>::New(isolate, constructor);
		args.GetReturnValue().Set(cons->NewInstance(argc, argv));
	}

	if(args.Length() < 1) {
		isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate,"Incorrect number of arguments, expected options at least")));
		args.GetReturnValue().Set(Undefined(isolate));
	}

	if(!args[0]->IsObject()) {
		isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate,"Expected options to be an object")));
		args.GetReturnValue().Set(Undefined(isolate));
	}

	Recognizer* instance = new Recognizer();

	// Add the configuration to the decoder instance
	Handle<Object> options = args[0]->ToObject();
	cmd_ln_t* config = BuildConfig(options);
	instance->ps = ps_init(config);


	// Initialize the callback functions
	Handle<Function> emptyFoo = Handle<Function>();
	if (args.Length() >= 2) {
		if(!args[1]->IsFunction()) {
			isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Expected hypothesis to be a function")));
			args.GetReturnValue().Set(Undefined(isolate));
		} else {
			// Set hypothesis from arguments
			Local<Function> hypothesis = Local<Function>::Cast(args[1]);
			instance->hypCallback.Reset(isolate, hypothesis);;
		}	
	} else {
		instance->hypCallback.Reset(isolate, emptyFoo);
	}

	// Init the other callback functions empty
	instance->hypFinalCallback.Reset(isolate, emptyFoo);
	instance->startCallback.Reset(isolate, emptyFoo);
	instance->stopCallback.Reset(isolate, emptyFoo);
	instance->speechDetectedCallback.Reset(isolate, emptyFoo);
	instance->silenceDetectedCallback.Reset(isolate, emptyFoo);
	instance->errorCallback.Reset(isolate, emptyFoo);

	// Set destructed to false initially
	instance->destructed = false;
	// Set processing to false initially
	instance->processing = false;
	// Set silenceDetection to true initially
	instance->silenceDetection = true;

	instance->Wrap(args.Holder());

	args.GetReturnValue().Set(args.Holder());
}

void Recognizer::Free(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

	if(instance->destructed == false) {
		instance->processing = false;
		ps_free(instance->ps);
	}
	instance->destructed = true;
}

void Recognizer::Reconfig(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

	if(args.Length() < 1) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Incorrect number of arguments, expected options at least")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Incorrect number of arguments, expected options at least"));
		args.GetReturnValue().Set(Undefined(isolate));
		return;
	}

	if(!args[0]->IsObject()) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Expected options to be an object")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Expected options to be an object"));
		args.GetReturnValue().Set(Undefined(isolate));
		return;
	}

	if(args.Length() >=2) {
		if(!args[1]->IsFunction()) {
			//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Expected hypothesis to be a function")));
			Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Expected hypothesis to be a function"));
			args.GetReturnValue().Set(Undefined(isolate));
			return;
		} else {
			// Set hypothesis from arguments
			Local<Function> hypothesis = Local<Function>::Cast(args[1]);
			instance->hypCallback.Reset(isolate, hypothesis);;
		}
	}

	// Remind state
	bool wasProcessing = instance->processing;
	instance->processing = false;

	// Add the configuration to the decoder instance
	Handle<Object> options = args[0]->ToObject();
	cmd_ln_t* config = BuildConfig(options);
	if(ps_reinit(instance->ps, config)<0) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Could not reinit decoder")));
		Recognizer::Error(instance, isolate, String::NewFromUtf8(isolate, "Could not reinit decoder"));
		args.GetReturnValue().Set(Undefined(isolate));
	} else {
		// Restart decoding when the decoder was running before
		if(wasProcessing) {
			Start(args);
		}
	}

	// Overwrite callback method
	Local<Function> cb = Local<Function>::Cast(args[1]);
	instance->hypCallback.Reset(isolate, cb);

	args.GetReturnValue().Set(args.Holder());
}

void Recognizer::SilenceDetection(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

	if(args.Length() < 1) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Incorrect number of arguments, expected boolean")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Incorrect number of arguments, expected boolean"));
		args.GetReturnValue().Set(Undefined(isolate));
		return;
	}

	if(!args[0]->IsBoolean()) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Expected enabled to be a boolean")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Expected enabled to be a boolean"));
		args.GetReturnValue().Set(Undefined(isolate));
		return;
	}

	instance->silenceDetection = args[0]->BooleanValue();
}

void Recognizer::On(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

	if(args.Length() < 2) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Incorrect number of arguments, expected event and callback")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Incorrect number of arguments, expected event and callback"));
		args.GetReturnValue().Set(Undefined(isolate));
		return;
	}

	if(!args[0]->IsString()) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Expected event to be a string")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Expected event to be a string"));
		args.GetReturnValue().Set(Undefined(isolate));
		return;
	}

	if(!args[1]->IsFunction()) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Expected callback to be a function")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Expected callback to be a function"));
		args.GetReturnValue().Set(Undefined(isolate));
		return;
	}

	String::Utf8Value event(args[0]);
	Local<Function> cb = Local<Function>::Cast(args[1]);

	if(strcmp(*event, "hyp")==0) {
		instance->hypCallback.Reset(isolate, cb);
	} else 
	if(strcmp(*event, "hypFinal")==0) {
		instance->hypFinalCallback.Reset(isolate, cb);
	} else
	if(strcmp(*event, "start")==0) {
		instance->startCallback.Reset(isolate, cb);
	} else
	if(strcmp(*event, "stop")==0) {
		instance->stopCallback.Reset(isolate, cb);
	} else
	if(strcmp(*event, "speechDetected")==0) {
		instance->speechDetectedCallback.Reset(isolate, cb);
	} else
	if(strcmp(*event, "silenceDetected")==0) {
		instance->silenceDetectedCallback.Reset(isolate, cb);
	} else
	if(strcmp(*event, "error")==0) {
		instance->errorCallback.Reset(isolate, cb);
	}
}

void Recognizer::Off(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

	if(args.Length() < 1) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Incorrect number of arguments, expected event and callback")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Incorrect number of arguments, expected event and callback"));
		args.GetReturnValue().Set(Undefined(isolate));
		return;
	}
	if(!args[0]->IsString()) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Expected event to be a string")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Expected event to be a string"));
		args.GetReturnValue().Set(Undefined(isolate));
		return;
	}
	String::Utf8Value event(args[0]);

	Handle<Function> emptyFoo = Handle<Function>();
	if(strcmp(*event, "hyp")==0) {
		instance->hypCallback.Reset(isolate, emptyFoo);
	} else 
	if(strcmp(*event, "hypFinal")==0) {
		instance->hypFinalCallback.Reset(isolate, emptyFoo);
	} else
	if(strcmp(*event, "start")==0) {
		instance->startCallback.Reset(isolate, emptyFoo);
	} else
	if(strcmp(*event, "stop")==0) {
		instance->stopCallback.Reset(isolate, emptyFoo);
	} else
	if(strcmp(*event, "speechDetected")==0) {
		instance->speechDetectedCallback.Reset(isolate, emptyFoo);
	} else
	if(strcmp(*event, "silenceDetected")==0) {
		instance->silenceDetectedCallback.Reset(isolate, emptyFoo);
	} else
	if(strcmp(*event, "error")==0) {
		instance->errorCallback.Reset(isolate, emptyFoo);
	}
}

void Recognizer::AddKeyphraseSearch(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

	if(args.Length() < 2) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Incorrect number of arguments, expected name and keyphrase")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Incorrect number of arguments, expected name and keyphrase"));
		args.GetReturnValue().Set(args.Holder());
		return;
	}

	if(!args[0]->IsString() || !args[1]->IsString()) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Expected both name and keyphrase to be strings")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Expected both name and keyphrase to be strings"));
		args.GetReturnValue().Set(args.Holder());
		return;
	}

	String::Utf8Value name(args[0]);
	String::Utf8Value keyphrase(args[1]);

	int result = ps_set_keyphrase(instance->ps, *name, *keyphrase);
	if(result < 0)
		Recognizer::Error(instance, isolate, String::NewFromUtf8(isolate, "Failed to add keyphrase search to recognizer"));
		//isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Failed to add keyphrase search to recognizer")));

	args.GetReturnValue().Set(args.Holder());
}

void Recognizer::AddKeywordsSearch(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

	if(args.Length() < 2) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Incorrect number of arguments, expected name and file")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Incorrect number of arguments, expected name and file"));
		args.GetReturnValue().Set(args.Holder());
		return;
	}

	if(!args[0]->IsString() || !args[1]->IsString()) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Expected both name and file to be strings")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Expected both name and file to be strings"));
		args.GetReturnValue().Set(args.Holder());
		return;
	}

	String::Utf8Value name(args[0]);
	String::Utf8Value file(args[1]);

	int result = ps_set_kws(instance->ps, *name, *file);
	if(result < 0)
		Recognizer::Error(instance, isolate, String::NewFromUtf8(isolate, "Failed to add keywords search to recognizer"));
		//isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Failed to add keywords search to recognizer")));

	args.GetReturnValue().Set(args.Holder());
}

void Recognizer::AddGrammarSearch(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

	if(args.Length() < 2) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Incorrect number of arguments, expected name and file")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Incorrect number of arguments, expected name and file"));
		args.GetReturnValue().Set(args.Holder());
	}

	if(!args[0]->IsString() || !args[1]->IsString()) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Expected both name and file to be strings")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Expected both name and file to be strings"));
		args.GetReturnValue().Set(args.Holder());
	}

	String::Utf8Value name(args[0]);
	String::Utf8Value file(args[1]);

	int result = ps_set_jsgf_file(instance->ps, *name, *file);
	if(result < 0)
		Recognizer::Error(instance, isolate, String::NewFromUtf8(isolate, "Failed to add grammar search to recognizer"));
		//isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Failed to add grammar search to recognizer")));

	args.GetReturnValue().Set(args.Holder());
}

void Recognizer::AddNgramSearch(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

	if(args.Length() < 2) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Incorrect number of arguments, expected name and file")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Incorrect number of arguments, expected name and file"));
		args.GetReturnValue().Set(args.Holder());
		return;
	}

	if(!args[0]->IsString() || !args[1]->IsString()) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Expected both name and file to be strings")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Expected both name and file to be strings"));
		args.GetReturnValue().Set(args.Holder());
		return;
	}

	String::Utf8Value name(args[0]);
	String::Utf8Value file(args[1]);

	int result = ps_set_lm_file(instance->ps, *name, *file);
	if(result < 0)
		Recognizer::Error(instance, isolate, String::NewFromUtf8(isolate, "Failed to add Ngram search to recognizer"));
		//isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Failed to add Ngram search to recognizer")));

	args.GetReturnValue().Set(args.Holder());
}

void Recognizer::GetSearch(Local<String> property, const PropertyCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.This());

	Local<Value> search = String::NewFromUtf8(isolate,ps_get_search(instance->ps));

	args.GetReturnValue().Set(search);
}

void Recognizer::SetSearch(Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& args) {
	Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.This());

	String::Utf8Value search(value);

	ps_set_search(instance->ps, *search);

	args.GetReturnValue().Set(args.Holder());
}

void Recognizer::Start(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

	if(instance->processing == false) {
		int result = ps_start_utt(instance->ps);
		if(result) {
			//isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Failed to start PocketSphinx processing")));
			Recognizer::Error(instance, isolate, String::NewFromUtf8(isolate, "Failed to start PocketSphinx processing"));
		} else {
			instance->processing = true;

			// Trigger start callback
			if(!instance->startCallback.IsEmpty()) {
				Handle<Value> argv[0] = {};
				Local<Function> cb = Local<Function>::New(isolate, instance->startCallback);
				cb->Call(isolate->GetCurrentContext()->Global(), 0, argv);
			}
		}
	} else {
		//isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "PocketSphinx processing seems to run already")));
		Recognizer::Error(instance, isolate, String::NewFromUtf8(isolate, "PocketSphinx processing seems to run already"));
	}

	args.GetReturnValue().Set(args.Holder());

	// Reset silence detection
	instance->speechDetected = false;
}

void Recognizer::Stop(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

	if(instance->processing == true) {

		// Fetch hyp with isFinal flag
		// ..and trigger hypFinal callback
		if(!instance->hypFinalCallback.IsEmpty()) {
			int32 isFinal;
			const char* hyp = ps_get_hyp_final(instance->ps, &isFinal);
			Handle<Value> argv[3] = { Null(isolate), hyp ? String::NewFromUtf8(isolate,hyp) : String::NewFromUtf8(isolate, ""), NumberObject::New(isolate, isFinal)};
		
			Local<Function> cb = Local<Function>::New(isolate, instance->hypFinalCallback);
			cb->Call(isolate->GetCurrentContext()->Global(), 3, argv);
		}

		// End the utterance
		int result = ps_end_utt(instance->ps);
		if(result){
			//isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Failed to end PocketSphinx processing")));
			Recognizer::Error(instance, isolate, String::NewFromUtf8(isolate, "Failed to end PocketSphinx processing"));
		} else {
			//cout << "Utt successfully stopped..." << endl;
			instance->processing = false;

			// Trigger stop callback
			if(!instance->stopCallback.IsEmpty()) {
				Handle<Value> argv[0] = {};
				Local<Function> cb = Local<Function>::New(isolate, instance->stopCallback);
				cb->Call(isolate->GetCurrentContext()->Global(), 0, argv);
			}
		}
	}

	args.GetReturnValue().Set(args.Holder());
}

void Recognizer::Restart(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

	if(instance->processing == true) {
		// Try stop processing
		int result = ps_end_utt(instance->ps);
		if(result) {
			//isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "Failed to restart PocketSphinx processing")));
			Recognizer::Error(instance, isolate, String::NewFromUtf8(isolate, "Failed to restart PocketSphinx processing"));
		} else {
			instance->processing = false;

			// Trigger stop callback
			if(!instance->stopCallback.IsEmpty()) {
				Handle<Value> argv[0] = {};
				Local<Function> cb = Local<Function>::New(isolate, instance->stopCallback);
				cb->Call(isolate->GetCurrentContext()->Global(), 0, argv);
			}

			// Restart it now that it's stopped
			Start(args);
		}
	} else {
		// Start processing
		Start(args);
	}

	args.GetReturnValue().Set(args.Holder());
}

void Recognizer::Write(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

	Handle<Object> buffer = args[0]->ToObject();

	if(!args.Length()) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Expected a data buffer to be provided")));
		Recognizer::Error(instance, isolate, String::NewFromUtf8(isolate, "Expected a data buffer to be provided"));
		args.GetReturnValue().Set(args.Holder());
	}

	if(!node::Buffer::HasInstance(buffer)) {
		/*
		Local<Value> argv[1] = { Exception::Error(String::NewFromUtf8(isolate, "Expected data to be a buffer")) };
		Local<Function> cb = Local<Function>::New(isolate, instance->hypCallback);
		cb->Call(isolate->GetCurrentContext()->Global(), 1, argv);*/
		Recognizer::Error(instance, isolate, String::NewFromUtf8(isolate, "Expected data to be a buffer"));
		args.GetReturnValue().Set(args.Holder());
	}

	AsyncData* data = new AsyncData();
	data->instance = instance;
	data->data = (int16*) node::Buffer::Data(buffer);
	data->length = node::Buffer::Length(buffer) / sizeof(int16);

	uv_work_t* req = new uv_work_t();
	req->data = data;

	uv_queue_work(uv_default_loop(), req, AsyncWorker, (uv_after_work_cb)AsyncAfter);

	args.GetReturnValue().Set(args.Holder());
}

void Recognizer::WriteSync(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

	// Skip the buffer when not processing
	if(instance->processing == false) {
		//cout << "Buffer recieved but not running..." << endl;
		return;
	}

	Handle<Object> buffer = args[0]->ToObject();

	if(!args.Length()) {
		Recognizer::Error(instance, isolate, String::NewFromUtf8(isolate, "Expected data to be a buffer"));
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Expected a data buffer to be provided")));
		return;
	}

	if(!node::Buffer::HasInstance(buffer)) {
		//Local<Value> argv[1] = { Exception::Error(String::NewFromUtf8(isolate, "Expected data to be a buffer")) };
		Recognizer::Error(instance, isolate, String::NewFromUtf8(isolate, "Expected data to be a buffer"));
		
		/*if(!instance->hypCallback.IsEmpty()) {
			Local<Function> cb = Local<Function>::New(isolate, instance->hypCallback);
			cb->Call(isolate->GetCurrentContext()->Global(), 1, argv);
		}*/
		return;
	}

	int16* data = (int16*) node::Buffer::Data(buffer);
	size_t length = node::Buffer::Length(buffer) / sizeof(int16);

	if(ps_process_raw(instance->ps, data, length, FALSE, FALSE) < 0) {
		/*Handle<Value> argv[1] = { Exception::Error(String::NewFromUtf8(isolate, "Failed to process audio data")) };

		// Trigger hyp callback
		if(!instance->hypCallback.IsEmpty()) {
			Local<Function> cb = Local<Function>::New(isolate, instance->hypCallback);
			cb->Call(isolate->GetCurrentContext()->Global(), 1, argv);
		}*/
		Recognizer::Error(instance, isolate, String::NewFromUtf8(isolate, "Failed to process audio data"));
		return;
	}

	int32 score;
	const char* hyp = ps_get_hyp(instance->ps, &score);

	// Silence detection
	if (instance->speechDetected == false && ps_get_in_speech(instance->ps)==1) {
		instance->speechDetected = true;
		// Trigger speechDetected callback
		if(!instance->speechDetectedCallback.IsEmpty()) {
			Handle<Value> argv[0] = {};
			Local<Function> cb = Local<Function>::New(isolate, instance->speechDetectedCallback);
			cb->Call(isolate->GetCurrentContext()->Global(), 1, argv);
		}
	}
	if (instance->speechDetected == true && ps_get_in_speech(instance->ps)==0) {
		// Trigger silenceDetected callback
		if(!instance->silenceDetectedCallback.IsEmpty()) {
			Handle<Value> argv[0] = {};
			Local<Function> cb = Local<Function>::New(isolate, instance->silenceDetectedCallback);
			cb->Call(isolate->GetCurrentContext()->Global(), 1, argv);
		}
		// Stop decoding when sd is enabled
		if (instance->silenceDetection) {
			Stop(args);
		}
	}

	Handle<Value> argv[3] = { Null(isolate), hyp ? String::NewFromUtf8(isolate,hyp) : String::NewFromUtf8(isolate, ""), NumberObject::New(isolate,score)};
	
	if(!instance->hypCallback.IsEmpty()) {
		Local<Function> cb = Local<Function>::New(isolate, instance->hypCallback);
		cb->Call(isolate->GetCurrentContext()->Global(), 3, argv);
	}

	args.GetReturnValue().Set(args.Holder());
}

void Recognizer::LookupWords(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

	if(args.Length() < 1) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Incorrect number of arguments, expected options at least")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Incorrect number of arguments, expected words"));
		args.GetReturnValue().Set(Undefined(isolate));
		return;
	}

	if(!args[0]->IsArray()) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Expected options to be an object")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Expected words to be an array"));
		args.GetReturnValue().Set(Undefined(isolate));
		return;
	}

	vector<string> result;
    Handle<Value> val;
    if (args[0]->IsArray()) {
		Handle<Array> jsArray = Handle<Array>::Cast(args[0]);
		for (unsigned int i = 0; i < jsArray->Length(); i++) {
			val = jsArray->Get(Integer::New(isolate, i)); 
			result.push_back(string(*String::Utf8Value(val)));
		}
	}

	vector<string> non_existing;
	vector<string> existing;
	vector<string> transcriptions;

    for (unsigned int i = 0; i < result.size(); ++i)
    {
    	const char* token = result.at(i).c_str();
    	const char* transcription = ps_lookup_word(instance->ps, token);

    	if(transcription!=NULL) {
    		existing.push_back(result.at(i));
    		transcriptions.push_back(string(transcription));
    	} else {
			non_existing.push_back(result.at(i));
    	}
    }

    Handle<Array> out_array = Array::New(isolate, int(non_existing.size()));
    Handle<Object> in_object = Object::New(isolate);
	for (unsigned int i = 0; i < non_existing.size(); ++i)
	{
		out_array->Set(i, String::NewFromUtf8(isolate, non_existing.at(i).c_str()));
	}
	for (unsigned int i = 0; i < existing.size(); ++i)
	{
		in_object->Set(
			String::NewFromUtf8(isolate, existing.at(i).c_str()),
			String::NewFromUtf8(isolate, transcriptions.at(i).c_str())
		);
	}

	Handle<Object> returnObject = Object::New(isolate);
	returnObject->Set(
		String::NewFromUtf8(isolate,"in"),
		in_object
	);
	returnObject->Set(
		String::NewFromUtf8(isolate,"out"),
		out_array
	);

    // Add the array to the return value
    args.GetReturnValue().Set(returnObject);
}

void Recognizer::AddWords(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	Recognizer* instance = node::ObjectWrap::Unwrap<Recognizer>(args.Holder());

	if(args.Length() < 1) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Incorrect number of arguments, expected options at least")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Incorrect number of arguments, expected words"));
		args.GetReturnValue().Set(Undefined(isolate));
		return;
	}

	if(!args[0]->IsObject()) {
		//isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Expected options to be an object")));
		Recognizer::TypeError(instance, isolate, String::NewFromUtf8(isolate, "Expected words to be an object"));
		args.GetReturnValue().Set(Undefined(isolate));
		return;
	}

	Handle<Object> words = Handle<Object>::Cast(args[0]);
	Local<Array> property_names = words->GetOwnPropertyNames();

	for (unsigned int i = 0; i < property_names->Length(); ++i) {
		Local<Value> key = property_names->Get(i);
		Local<Value> value = words->Get(key);

		if (key->IsString() && value->IsString()) {
			String::Utf8Value utf8_key(key);
			String::Utf8Value utf8_value(value);
			//cout << *utf8_key << "->" << *utf8_value << endl;
			int update = i == property_names->Length()-1 ? 1 : 0;
			ps_add_word(instance->ps, *utf8_key, *utf8_value, update);
		}
	}
}

void Recognizer::AsyncWorker(uv_work_t* request) {
	Isolate* isolate = Isolate::GetCurrent();
	AsyncData* data = reinterpret_cast<AsyncData*>(request->data);

	if(ps_process_raw(data->instance->ps, data->data, data->length, FALSE, FALSE)) {
		data->hasException = TRUE;
		data->exception = Exception::Error(String::NewFromUtf8(isolate, "Failed to process audio data"));
		return;
	}

	int32 score;
	const char* hyp = ps_get_hyp(data->instance->ps, &score);

	data->score = score;
	data->hyp = hyp;
}

void Recognizer::AsyncAfter(uv_work_t* request) {
	Isolate* isolate = Isolate::GetCurrent();
	AsyncData* data = reinterpret_cast<AsyncData*>(request->data);


	if(data->hasException) {
		Handle<Value> argv[1] = { data->exception };
		if(!data->instance->hypCallback.IsEmpty()) {
			Local<Function> cb = Local<Function>::New(isolate, data->instance->hypCallback);
			cb->Call(isolate->GetCurrentContext()->Global(), 1, argv);
		}
	} else {
		Handle<Value> argv[3] = { Null(isolate), data->hyp ? String::NewFromUtf8(isolate,data->hyp) : String::NewFromUtf8(isolate, ""), NumberObject::New(isolate,data->score)};
		if(!data->instance->hypCallback.IsEmpty()) {
			Local<Function> cb = Local<Function>::New(isolate, data->instance->hypCallback);
			cb->Call(isolate->GetCurrentContext()->Global(), 3, argv);
		}
	}
}

Local<Value> Recognizer::Default(Local<Value> value, Local<Value> fallback) {
	if(value->IsUndefined()) return fallback;
	return value;
}

cmd_ln_t* Recognizer::BuildConfig(Handle<Object> options) {

	Isolate* isolate = Isolate::GetCurrent();

	// Create an empty command line config
	cmd_ln_t* config = cmd_ln_init(NULL, ps_args(), TRUE, NULL);

	// Add default parameters if they do not exist
	options->Set(
		String::NewFromUtf8(isolate,"-hmm"),
		Default(options->Get(String::NewFromUtf8(isolate,"-hmm")), String::NewFromUtf8(isolate, MODELDIR "/en-us/en-us"))
	);
	options->Set(
		String::NewFromUtf8(isolate,"-dict"),
		Default(options->Get(String::NewFromUtf8(isolate,"-dict")), String::NewFromUtf8(isolate, MODELDIR "/en-us/cmudict-en-us.dict"))
	);
	// For some reason when passing -samprate or -agcthresh (maybe more) the values will be set wrong and some weird values are added
	options->Set(
		String::NewFromUtf8(isolate,"-samprate"),
		Default(options->Get(String::NewFromUtf8(isolate,"-samprate")), Number::New(isolate, 44100))
	);
	options->Set(
		String::NewFromUtf8(isolate,"-nfft"),
		Default(options->Get(String::NewFromUtf8(isolate,"-nfft")), Number::New(isolate, 2048))
	);

	// Add all parameters to the config
	Local<Array> propertyNames = options->GetOwnPropertyNames();
	for (uint32_t i = 0; i < propertyNames->Length(); ++i)
	{
		Local<Value> key = propertyNames->Get(i);
		Local<Value> val = options->Get(key);

		if (key->IsString()) {

			String::Utf8Value utf8_key(key);
			// Check if the key is valid
			anytype_t *ps_val;
			ps_val = cmd_ln_access_r(config, *utf8_key);
			if (ps_val == NULL) {
				Local<String> err = String::Concat(String::NewFromUtf8(isolate, "Unknown pocketsphinx argument: "), String::NewFromUtf8(isolate, *utf8_key));
				isolate->ThrowException(Exception::TypeError(err));
			}

			// Add String values
			if (val->IsString()) {
				String::Utf8Value utf8_val(val);
				cmd_ln_set_str_r(config, *utf8_key, *utf8_val);

			// Add numeric values
			} else if(val->IsNumber()) {
				double num_val = double(val->NumberValue());
				// Check if the number is a float or int
				bool isInt = (val->IsInt32() || val->IsUint32());
				// Also some numbers have to be passed as float, otherwise ps configuration will crash
				if (isInt && 
					strcmp(*utf8_key, "-samprate")!=0 && 
					strcmp(*utf8_key, "-lw")!=0 &&
					strcmp(*utf8_key, "-fwdflatlw")!=0 &&
					strcmp(*utf8_key, "-pip")!=0 &&
					strcmp(*utf8_key, "-uw")!=0 &&
					strcmp(*utf8_key, "-vad_threshold")!=0 &&
					strcmp(*utf8_key, "-bestpathlw")!=0)
				{					
					cmd_ln_set_int_r(config, *utf8_key, (long)num_val);
				} else {
					cmd_ln_set_float_r(config, *utf8_key, num_val);
				}

			// Add boolean values
			} else if(val->IsBoolean()) {
				bool bool_val = bool(val->BooleanValue());
				cmd_ln_set_boolean_r(config, *utf8_key, bool_val);

			// Some other unknown value type was found
			} else {
				Local<String> err = String::Concat(String::NewFromUtf8(isolate, "Unknown value type for key: "), String::NewFromUtf8(isolate, *utf8_key));
				isolate->ThrowException(Exception::TypeError(err));
			}
		} else {
			isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "All argument keys must be strings")));
		}
	}

	return config;
}

void Recognizer::Error(Recognizer* instance, Isolate* isolate, const Handle<v8::String> msg) {
	if(!instance->errorCallback.IsEmpty()) {
		Handle<Value> argv[1] = { Exception::Error(msg) };
		Local<Function> cb = Local<Function>::New(isolate, instance->errorCallback);
		cb->Call(isolate->GetCurrentContext()->Global(), 1, argv);
	}
}
void Recognizer::TypeError(Recognizer* instance, Isolate* isolate, const Handle<v8::String> msg) {
	if(!instance->errorCallback.IsEmpty()) {
		Handle<Value> argv[1] = { Exception::TypeError(msg) };
		Local<Function> cb = Local<Function>::New(isolate, instance->errorCallback);
		cb->Call(isolate->GetCurrentContext()->Global(), 1, argv);
	}
}

void Recognizer::FromFloat(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);

	if(!args.Length()) {
		isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate,"Expected a data buffer to be provided")));
		args.GetReturnValue().Set(args.Holder());
	}

	if(!node::Buffer::HasInstance(args[0])) {
		isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate,"Expected data to be a buffer")));
		args.GetReturnValue().Set(args.Holder());
	}

	float* data = reinterpret_cast<float*>(node::Buffer::Data(args[0]));
	size_t length = node::Buffer::Length(args[0]) / sizeof(float);

	Local<Object> slowBuffer = node::Buffer::New(length * sizeof(int16));
	int16* slowBufferData = reinterpret_cast<int16*>(node::Buffer::Data(slowBuffer));

	//args.GetReturnValue().Set(args.Holder());

	for(size_t i = 0; i < length; i++)
		slowBufferData[i] = data[i] * 32768;

	// Courtesy of http://sambro.is-super-awesome.com/2011/03/03/creating-a-proper-buffer-in-a-node-c-addon/
	Local<Object> globalObj = isolate->GetCurrentContext()->Global();
	Local<Function> bufferConstructor = Local<Function>::Cast(globalObj->Get(String::NewFromUtf8(isolate, "Buffer")));
	Handle<Value> constructorArgs[3] = { slowBuffer, Integer::New(isolate, length), Integer::New(isolate, 0) };
	Local<Object> actualBuffer = bufferConstructor->NewInstance(3, constructorArgs);
	args.GetReturnValue().Set(actualBuffer);
}