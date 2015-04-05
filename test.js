var a = require('lib/pdf-fill-form.js');
var fs = require('fs');

console.log(a.readSync('ilmo.pdf'));

// var myPdf = a.writeFields('ilmo.pdf', { "Sivun1Tekstit.5.0": "Test Oy", "Sivun2bTekstit.0.25": "jeps", "b": "c" }, { "save": "imgpdf" });
// fs.writeFile("test123.pdf", myPdf, function(err) {
//   if(err) {
//       return console.log(err);
//   }
//   console.log("The file was saved!");
// }); 

a.writeAsync('ilmo.pdf', { "Sivun1Tekstit.5.0": "Test Oy", "Sivun2bTekstit.0.25": "jeps", "b": "c" }, { "save": "imgpdf" }, function(err, result) {
  fs.writeFile("test1234.pdf", result, function(err) {
    if(err) {
        return console.log(err);
    }
    console.log("The file was saved!");
  }); 
});

console.log('jee');