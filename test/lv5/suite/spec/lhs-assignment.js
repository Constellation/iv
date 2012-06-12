describe("lhs assignment", function() {
  it("literal assignment", function() {
    var i = 0;
    try {
      // should be early error
      eval('(function () { i = 10 = 20 }()');
    } catch (e) {
      expect(i).toBe(0);
    }
  });

  it("literal assignment with +", function() {
    var i = 0;
    try {
      // should be early error
      eval('(function () { i = 10 += 20 }()');
    } catch (e) {
      expect(i).toBe(0);
    }
  });

  it("assignment to function call", function() {
    expect(eval('(function () { print() = 10; }); true')).toBe(true);
  });

  it("assignment to constructor call", function() {
    var i = 0;
    try {
      eval('(function () { new print() = 10; });');
    } catch (e) {
      expect(i).toBe(0);
    }
  });
});
