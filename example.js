var pdfFormFill = require('lib/pdf-fill-form.js');
var fs = require('fs');

// Show fields
var formFields = pdfFormFill.readSync('test.pdf');
console.log(formFields);

// Write fields
pdfFormFill.writeAsync('test.pdf', { 'myField1': 'myField1 fill text' }, { 'save': 'imgpdf' }, function(err, result) {
  if (err) {
      return console.log(err);
  }

  fs.writeFile('test_filled_images.pdf', result, function(err) {
    if (err) {
        return console.log(err);
    }
    console.log('The file was saved!');
  });
});
