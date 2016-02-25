function getRandomIntByDigitCount(d) {
  var min = 10 * (d - 1),
    max = Math.pow(10, d) - 1;
  return Math.round(Math.random() * (max - min + 1)) + min;
}

function getRandomName(d) {
  var min = 10 * (d - 1),
    max = Math.pow(10, d) - 1;
  return Math.round(Math.random() * (max - min + 1)) + min + "";

}

function getRandomInt(min, max) {
  if (!max) {
    max = min;
  }
  return Math.floor(Math.random() * (max - min + 1)) + min;
}



function ipaddress(){
  return getRandomInt(1, 255) + '.' +
    getRandomInt(1, 255) + '.' +
    getRandomInt(1, 255) + '.' +
    getRandomInt(1, 255);
}

function syllable(length) {
  // based on chance.js implementation
  if (!length || length < 2) {
    length = 3;
  }

  var consonants = 'bcdfghjklmnprstvwz', // consonants except hard to speak ones
    vowels = 'aeiou', // vowels
    all = consonants + vowels, // all
    text = '',
    chr;
  // I'm sure there's a more elegant way to do this, but this works
  // decently well.
  for (var i = 0; i < length; i++) {
    if (i === 0) {
      // First character can be anything
      chr = all[getRandomInt(0, all.length - 1)];
    } else if (consonants.indexOf(chr) === -1) {
      // Last character was a vowel, now we want a consonant
      chr = consonants[getRandomInt(0, consonants.length - 1)];
    } else {
      // Last character was a consonant, now we want a vowel
      chr = vowels[getRandomInt(0, vowels.length - 1)];
    }
    text += chr;
  }
  return text;
}

module.exports = {
  getRandomIntByDigitCount: getRandomIntByDigitCount,
  getRandomInt: getRandomInt,
  //repeatCount: repeatCount,
  getRandomName: getRandomName,
  ipaddress: ipaddress,
  syllable: syllable
};