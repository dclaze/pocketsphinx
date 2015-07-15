# PocketSphinx for Node.js
**A Node.js Binding for PocketSphinx**

This module aims to allow basic speech recognition on portable devices through the use of PocketSphinx. It is possible to either use buffer chunks for live recognition or pass audio buffers with the complete audio to PocketSphinx to decode.

## Prerequisites

To build the node extension you have to install Sphinxbase and PocketSphinx from the repositories preferably:

* [Sphinxbase repository](https://github.com/cmusphinx/sphinxbase)
* [PocketSphinx repository](https://github.com/cmusphinx/pocketsphinx)

## Example

```javascript
var PocketSphinx = require('pocketsphinx');

var ps = new PocketSphinx.Recognizer({
	'-hmm': '/file/path',
	'-dict': '/file/path',
	'-nfft': 512,
	'-remove_silence': false	
});

ps.on('hyp', function(err, hypothesis, score){
	if(err) console.error(err);
	console.log('Hypothesis: ', hypothesis);
});

ps.addKeyphraseSearch("keyphrase_name", "keyphrase");
ps.addKeywordSearch("keyword_name", "/file/path");
ps.addGrammarSearch("grammar_name", "/file/path");
ps.addNgramSearch("ngram_name", "/file/path");

ps.search = "keyphrase_name";

ps.start();
ps.write(data);
ps.stop();
```

## Options

You can pass any valid PocketSphinx parameter you could e.g. pass to pocketsphinx_continuous but you have to assure you're using the correct type for the parameter. If you are for example pass '-samprate' as a string like '44100.0' the configuration will fail. In the following list you can see some of the probably most required options with their required data type:

option | type | default | description
-------|------|---------|------------
`-samprate` | float | `44100.0` | The sample rate of the passed data
`-hmm` | string | `modelDirectory` + `"/en-us/en-us"` | The hmm model directory
`-dict` | string | `modelDirectory` + `"/en-us/cmudict-en-us.dict"` | The dictionary file directory
`-nfft` | int | `2048` | The nfft value
For more options you can look into the manual of pocketsphinx_continuous with `$ man pocketsphinx_continuous`


## Methods

The PocketSphinx Object itself has the properties

* `Recognizer(options, [hyp])` - Creates a new Recognizer instance
* `modelDirectory` - The default model directory
* `fromFloat(buffer)` - Resamples javascript audio buffers to use with PocketSphinx

A Recognizer instance has the following methods:

* `on(event, function)` - Attaches an event handler
* `off(event)` - Removes an event handler
* `start()` - Starts the decoder
* `stop()` - Stops the decoder
* `restart()` - Restarts the decoder
* `reconfig(options, [hyp])` - Reconfigures the decoder without having to reload it
* `silenceDetection(enabled)` - Disables or enables silence detection (Default: enabled)
* `addKeyphraseSearch(name, keyphrase)` - Adds a keyphrase search
* `addKeywordsSearch(name, keywordFile)` - Adds a keyword search
* `addGrammarSearch(name, jsgfFile)` - Adds a jsgf search
* `addNgramSearch(name, nGramFile)` - Adds a nGram search
* `write(buffer)` - Decodes a complete audio buffer
* `writeSync(buffer)` - Decodes the next audio buffer chunk

## Events

The following events are currently supported

event | parameters | description
------|------------|------------
`hyp` | `error, hypothesis, score` | When a hypothesis is available. `score` is the path score corresponding to returned string.
`hypFinal` | `error, hypothesis, isFinal` | When decoding stopped. `isFinal` indicates if hypothesis has reached final state in the grammar.
`start` | none | When decoding started.
`stop` | none | When decoding stopped.
`speechDetected` | none | When speech was detected the first time.
`silenceDetected` | none | When silence was detected after speech.


## Specify a search

To specify a search you can use one of the add functions mentioned in the methods section above and then add the name to the instance's search accessor like so:

```javascript
ps.addGrammarSearch('someSearch', 'digits.gram');
ps.search = 'someSearch';
ps.start();
```

Or you can pass e.g. a language model file, a jsgf grammar or a keyword file directly with the Recognizer options or reconfigure the Recognizer with such options at runtime:

```javascript
var ps = new PocketSphinx.Recognizer({
	'-lm': '/path/myFancyLanguageModel' // This adds an nGram search
});
ps.on('hyp', function(err, hypothesis, score) {
	if(err) console.error(err);
	console.log('Hypothesis: ', hypothesis);
});
ps.start();
```