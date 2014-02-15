describe("toString", function() {
  it("don't call toString", function() {
    var flag = true;
    var obj = {
      toString: function toString() {
        flag = false;
        return 'test';
      }
    };
    try {
      null[obj];
    } catch (e) {
      expect(flag).toBe(true);
    }
    try {
      undefined[obj];
    } catch (e) {
      expect(flag).toBe(true);
    }
  });
});
