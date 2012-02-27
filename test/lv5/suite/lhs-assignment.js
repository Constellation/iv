function test1() {
  var i = 0;
  try {
    i = 10 = 20;
  } catch (e) {
    return i === 0;
  }
}
function test2() {
  var i = 0;
  var obj = {
  };
  try {
    i = 10 += 20;
  } catch (e) {
    return i === 0;
  }
}
function test3() {
  var i = 0;
  var obj = {
    flag: false,
    valueOf: function() {
      obj.flag = true;
      return 20;
    }
  };
  try {
    i = 10 += obj;
  } catch (e) {
    return i === 0 && obj.flag;
  }
}

function test4() {
  var i = 0, obj;
  try {
    i = (obj = {
      flag: false,
      valueOf: function() {
        obj.flag = true;
        return 20;
      }
    }) += 10;
  } catch (e) {
    return i === 0 && obj.flag;
  }
}

function test5() {
  var i = 0, obj;
  try {
    i = (obj = {
      flag: false,
      valueOf: function() {
        this.flag = true;
        throw new Error;
      }
    }) += 10;
  } catch (e) {
    return i === 0 && obj.flag;
  }
}

test1() && test2() && test3() && test4() && test5();
