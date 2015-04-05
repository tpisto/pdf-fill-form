(function () {    
    "use strict";
    try {
        try {
            module.exports = require(__dirname +'/../build/Debug/pdf_fill_form');
        } catch (e) {
            module.exports = require(__dirname+'/../build/Release/pdf_fill_form');
        }
    } catch (e) {
        console.log(e);
        module.exports = require(__dirname+'/../build/default/pdf_fill_form');
    }
})();