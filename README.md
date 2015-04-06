<img src="http://res.cloudinary.com/tpisto/image/upload/v1428317033/pdf-fill-form-logo_rlfj7o.png" width="400"><br/>
**pdf-fill-form** is Node.js native C++ library for filling PDF forms. Created PDF file is returned back as node.js Buffer object for further processing or saving. Library offers methods to return filled PDF also as PDF file where pages are converted to images.
##Examples
To **read** all form fields:  

```javascript
var pdfFillForm = require('pdf-fill-form');

var pdfFields = pdfFillForm.readSync('test.pdf');
console.log(pdfFields);
```
To **write** form fields (synchronous) to PDF:

```javascript
var pdfFillForm = require('pdf-fill-form');
var fs = require('fs');

// Use here the field names you got from read
var pdf = pdfFillForm.writeSync('test.pdf', 
	{ "myField": "myField fill value" }, { "save": "pdf" } );
fs.writeFileSync('filled_test.pdf', pdf);
```
To **write** form fields (aynchronous) to PDF:

```javascript
var pdfFillForm = require('pdf-fill-form');
var fs = require('fs');

// Use here the field names you got from read
pdfFillForm.writeAsync('test.pdf', 
	{ "myField": "myField fill value" }, { "save": "pdf" }, 
	function(err, pdf) {
		fs.writeFile("filled_test.pdf", pdf, function(err){});
	}
);
```
To **write** form fields to PDF where pages are converted to images:  

Use parameter { "save": "imgpdf" }

##Installation

###OS X
Preferable method to install library dependencies is via [Homebrew](http://brew.sh/)

```
$ brew install cairo poppler
```
After dependencies are successfully installed, you can install the library:

```
$ npm install pdf-fill-form
```  
> Homebrew users who get error regarding xcb-shm  
> The fix is to add this to your bash profile / environment: export PKG_CONFIG_PATH=/opt/X11/lib/pkgconfig

###Linux (Ubuntu/Debian)
```
$ sudo apt-get install libpoppler-qt4-dev libcairo2-dev
$ npm install pdf-fill-form
```


##Authors
- [Tommi Pisto](https://github.com/tpisto)

## License
MIT  

NOTE ABOUT LIBRARY DEPENDENCIES!   
*Poppler has* ***GPL*** *license.* Cairo has LGPL.