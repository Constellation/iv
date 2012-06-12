describe("lhs assignment", function() {
  it("literal assignment", function() {
    var i = 0;
    try {
      eval('i = 10 = 20');
    } catch (e) {
      expect(i).toBe(0);
    }
  });

  it("literal assignment with +", function() {
    var i = 0;
    var obj = {
    };
    try {
      eval('i = 10 += 20');
    } catch (e) {
      expect(i).toBe(0);
    }
  });
});
