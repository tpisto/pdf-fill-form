var a = require('lib/piston-poppler.js');
var fs = require('fs');

var myPdf = a.writeFields('test.pdf', { "Sivun1Tekstit.0": 'Test Oy', "b": "c" } );
fs.writeFile("test123.pdf", myPdf, function(err) {
  if(err) {
      return console.log(err);
  }
  console.log("The file was saved!");
}); 
