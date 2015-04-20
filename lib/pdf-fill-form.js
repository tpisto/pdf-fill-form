(function () {
    "use strict";

    var makePromises = function(myLib) {

        // Read promise (sync)
        myLib.read = function(fileName) {
            return new Promise(function(resolve, reject) {
                try {
                    var myFile = myLib.readSync(fileName);
                    resolve(myFile);
                } 
                catch(error) {
                    reject(error);
                }
            });
        }

        // Write promise (async)
        myLib.write = function(fileName, fields, params) {
            return new Promise(function(resolve, reject) {
                try {
                    myLib.writeAsync(fileName, fields, params, function(err, result) {
                        if(err) { reject(err); }
                        else {
                            resolve(result);
                        }
                    });
                } 
                catch(error) {
                    reject(error);
                }
            });
        }        

        return myLib;
    }

    try {
        try {
            module.exports = makePromises(require(__dirname +'/../build/Debug/pdf_fill_form'));
        } catch (e) {
            module.exports = makePromises(require(__dirname+'/../build/Release/pdf_fill_form'));
        }
    } catch (e) {
        console.log(e);
        module.exports = makePromises(require(__dirname+'/../build/default/pdf_fill_form'));
    }
})();