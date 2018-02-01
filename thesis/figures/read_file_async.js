const fs = require('fs');

function callback(err, file) {
  if (err) throw err;
  console.log(file);
}

fs.readFile('path/to/file', callback);
console.log('end of code');
