var a = require('lib/piston-poppler.js')

console.log(a.writeFields('test.pdf','moi.pdf', 
  { "Sivun1Tekstit.0": 'Test Oy', 
    "b": "c"
  } 
))

