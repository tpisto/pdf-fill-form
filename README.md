<img src="http://res.cloudinary.com/tpisto/image/upload/v1428317033/pdf-fill-form-logo_rlfj7o.png" width="400"><br/>
PDF Fill Form (**pdf-fill-form**) is Node.js native C++ library for filling PDF forms. Created PDF file is returned back as Node.js Buffer object for further processing or saving - *whole process is done in memory*. Library offers methods to return filled PDF also as PDF file where pages are converted to images.

Libary uses internally Poppler QT5 for PDF form reading and filling. Cairo is used for PDF creation from page images (when parameter { "save": "imgpdf" } is used). 

## Features

* Supports reading and writing the following PDF form field types: TextField, Checkbox, Radio button

* You can write following files:
	* PDF
	* PDF where pages are converted to images
* All the work is done in memory - no temporary files created
* Results are returned in Node.js Buffer -object
* Not using the PDFtk -executable - instead we use the Poppler library

## Examples
### Using promises
Read from file
```javascript
var pdfFillForm = require('pdf-fill-form');

pdfFillForm.read('test.pdf')
.then(function(result) {
    console.log(result);
}, function(err) {
	console.log(err);
});
```

Read from file buffer
```javascript
var pdfFillForm = require('pdf-fill-form');

pdfFillForm.readBuffer(fs.readFileSync('test.pdf'))
.then(function(result) {
    console.log(result);
}, function(err) {
	console.log(err);
});
```

Write from file
```javascript
var pdfFillForm = require('pdf-fill-form');
var fs = require('fs');

pdfFillForm.write('test.pdf', { "myField": "myField fill value" }, { "save": "pdf", 'cores': 4, 'scale': 0.2, 'antialias': true } )
.then(function(result) {
	fs.writeFile("test123.pdf", result, function(err) {
		if(err) {
	   		return console.log(err);
	   	}
	   	console.log("The file was saved!");
	}); 
}, function(err) {
  	console.log(err);
});
```

Write from file buffer
```javascript
var pdfFillForm = require('pdf-fill-form');
var fs = require('fs');

pdfFillForm.writeBuffer(fs.readFileSync('test.pdf'), { "myField": "myField fill value" }, { "save": "pdf", 'cores': 4, 'scale': 0.2, 'antialias': true } )
.then(function(result) {
	fs.writeFile("test123.pdf", result, function(err) {
		if(err) {
	   		return console.log(err);
	   	}
	   	console.log("The file was saved!");
	}); 
}, function(err) {
  	console.log(err);
});
```
### Using callbacks
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

### Notes about using radio buttons (@mttchrry)
Just set the radio button field Value to the caption of the item you want to select.

For example if you have a radio button for gender called "Gender1" with options captioned as "Male" or "Female" then passing in the field {"Gender1": "Male"} will select the male radio button, as expected.

## Installation

### macOS

Preferable method to install library dependencies is via [Homebrew](http://brew.sh/)

**First, make sure the XCode Command Line Tools are installed correctly**

```
xcode-select --install
```

**Also make sure pkg-config is installed correctly**

```
brew install pkg-config
```

**Then install the [Poppler](https://formulae.brew.sh/formula/poppler) and [Cairo](https://formulae.brew.sh/formula/cairo) formulae using ```brew```**

_NB: Poppler contains all the necessary ```qt``` version 5 depencencies._

```
brew install poppler cairo
```

**Also export environnement variables used by the compiler during the ```pdf-fill-form``` installation**
```
export LDFLAGS=-L/usr/local/opt/qt/lib
export CPPFLAGS=-I/usr/local/opt/qt/include
export PKG_CONFIG_PATH=/usr/local/opt/qt/lib/pkgconfig
```

_NB: You may also append the above ```export``` commands to your `~/.bash_profile` or `~/.zshrc` file for persistency._

**Finally, install the package using npm or yarn**
```
$ npm install pdf-fill-form
```
```
$ yarn add pdf-fill-form
```

#### Troubleshooting

> Homebrew users who get error regarding xcb-shm  
> The fix is to add this to your bash profile / environment: export PKG_CONFIG_PATH=/opt/X11/lib/pkgconfig


### Linux - Ubuntu (trusty)
```
$ sudo apt-get install libpoppler-qt5-dev libcairo2-dev
$ npm install pdf-fill-form
```
### Linux - Debian (jessie)

To be sure to have the required packages, re-synchronize the package index files from their sources :
```
$ sudo apt-get update
```
Then install packages :

```
$ sudo apt-get install libcairo2-dev libpoppler-qt5-dev 
$ npm install pdf-fill-form
```

I mostly recommand to install this package to have better support with fonts :
```
$ sudo apt-get install poppler-data
```

### Linux - CENTOS 8

First, enable the PowerTools repository:
```
$ yum config-manager --set-enabled PowerTools
```

Install the dependencies :
```
$ dnf install poppler-qt5-devel cairo cairo-devel
```

Then install the package using npm or yarn :
```
$ npm install pdf-fill-form
```
```
$ yarn add pdf-fill-form
```

### Windows

Not currently supported

## Todo
* Tests
* Refactoring
* Support for other form field types than CheckBox, Radio button and TextField

## Changelog


v5.0.0 (6.11.2019)
- New options startPage and endPage for the imgpdf feature to limit which pages are generated.  Page numbers indexed from 0 (by Derick Naef @ochimo).
- Allow source to come in a a buffer an not a filename (by Dustin Harmon @dfharmon)
- Support for Node 13 (by @florianbepunkt)

v4.1.0 (10.9.2018)
- Support for Node 10 (by @florianbepunkt)

v4.0.0 (14.12.2017)
- #45 Set radio button "value" to the poppler button state (by Albert Astals Cid @tsdgeos)
- Added feature allowing for parallelization of the imgpdf feature, also allows for settings scale and whether antialiasing should be used (by Albert Astals Cid @tsdgeos).

v3.3.0 (14.12.2017)
- #49 Set radio button "value" to the poppler button state (by Mihai Saru @MitzaCoder)

v3.2.0 (24.5.2017)
- Support for radio buttons (by Matt Cherry @mttchrry)

## Authors
- [Tommi Pisto](https://github.com/tpisto)

## Contibutors
- Ethan Goldblum
- Tyler Iguchi
- Rob Davarnia
- Fabrice Ongenae
- Juwan Yoo
- Andreas Gruenbacher
- Andrei Dracea
- Emil Sedgh
- Matt Cherry
- Mario Ferreira
- Mihai Saru @MitzaCoder
- Albert Astals Cid @tsdgeos
- Florian Bischoff @florianbepunkt
- Derick Naef @ochimo
- Dustin Harmon @dfharmon

## License
MIT  

NOTE ABOUT LIBRARY DEPENDENCIES!
*Poppler has* ***GPL*** *license.* Cairo has LGPL.
