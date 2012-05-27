describe("lhs assignment", function() {
  it("literal assignment", function() {
    var i = 0;
    try {
      i = 10 = 20;
    } catch (e) {
      expect(i).toBe(0);
    }
  });

  it("literal assignment with +", function() {
    var i = 0;
    var obj = {
    };
    try {
      i = 10 += 20;
    } catch (e) {
      expect(i).toBe(0);
    }
  });

  it("valueOf call by assignment with +", function() {
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
      expect(i).toBe(0);
      expect(obj.flag).toBe(true);
    }
  });

  it("valueOf call by assignment with literal 10 +", function() {
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
      expect(i).toBe(0);
      expect(obj.flag).toBe(true);
    }
  });

  it("valueOf call by assignment with literal 10 + throwing error", function() {
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
      expect(i).toBe(0);
      expect(obj.flag).toBe(true);
    }
  });
});
