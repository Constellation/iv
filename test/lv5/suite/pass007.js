function test001() {
  function raise() {
    throw new Error();
  }
  try {
    var obj = {
      t: raise()
    };
  } catch (e) {
    return obj === undefined;
  }
  return false;
}

function test002() {
  function raise() {
    throw new Error();
  }
  try {
    var ary = [ raise() ];
  } catch (e) {
    return ary === undefined;
  }
  return false;
}

test001() && test002();
