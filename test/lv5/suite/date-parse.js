function parseAssert(str) {
  return Date.parse(str) === 946684800000;
}

parseAssert('1999-12-31T23:59:60.000');
