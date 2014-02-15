describe("rhs assignment", function() {
  it("numeric literal assignment in call site arguments", function() {
    var i = 20;
    function inner(i) {
      return i;
    }
    inner(i + inner(i = 30));
    expect(inner(i)).toBe(30);
  });

  it("function assignment in call site arguments", function() {
    var inner = function(i, j) { return i + j + 10; };
    expect(inner(10, 10) + inner((inner = function() { return 20 })(), inner())).toBe(80);
  });

  it("numeric literal assignment in operator +", function() {
    var x = 10;
    expect(x + (x = 20) + x).toBe(50);
  });

  it("numeric literal assignment in conditional operator with paren", function() {
    var x = 10;
    var y = 20;
    expect(x + ((y === 20) ? (x = 20) : x)).toBe(30);
  });

  it("numeric literal assignment in conditional operator", function() {
    var x = 10;
    var y = 20;
    expect(x + (y === 20) ? (x = 20) : x).toBe(20);
  });

  it("assignment in call site arguments with a lot of arguments", function() {
    var inner = function() { return 10; };
    expect(inner() + inner(inner = function() { return 20; }, 1, 2, 3, 4, 5)).toBe(20);
  });

  it("assignment with increment", function() {
    var i = 20;
    expect(i + i++ + i).toBe(61);
  });

  it("complex assignment in arguments", function() {
      var a = 10, b = 20, c = 30, d = 40;
      function add(lhs, rhs) {
        return lhs + rhs;
      }
      a = add(a, b = 40);
      expect(a).toBe(50);
      expect(b).toBe(40);
      expect(c).toBe(30);
      expect(d).toBe(40);
  });
});
