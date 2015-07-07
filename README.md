# PocketSphinx for Node.js
**A Node.js Binding for PocketSphinx**

This module aims to allow basic speech recognition on portable devices through the use of PocketSphinx. Much of its API is based on the [PocketSphinx WP Demo](https://github.com/cmusphinx/pocketsphinx-wp-demo) application and it aims to provide enough functionality for basic speech recognition tasks.

## Example

```javascript
var PocketSphinx = require('pocketsphinx');

var ps = new PocketSphinx.Recognizer({
	hmm: '/file/path',
	dict: '/file/path',
	samprate: 16000,
	nfft: 512
}, function(err, hypothesis, score) {

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