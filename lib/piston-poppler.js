(function () {
    
    "use strict";
    try {
        try {
            module.exports = require(__dirname +'/../build/Debug/piston-poppler');
        } catch (e) {
            module.exports = require(__dirname+'/../build/Release/piston-poppler');
        }
    } catch (e) {
        console.log(e);
        module.exports = require(__dirname+'/../build/default/piston-poppler');
    }

})();