describe("Arguments", function() {
  it("duplicate parameter names", function() {
    function test(a, a) {
      expect(arguments[0]).toBe(0);
      expect(arguments[1]).toBe(1);
      a = 20;
      expect(arguments[0]).toBe(0);
      expect(arguments[1]).toBe(20);
    }
    test(0, 1);
  });

  it("not hiding name `arguments`", function() {
    var v = function arguments() {
      return arguments;
    };
    expect(v()).not.toBe(v);
  });

  it("not exposed function name", function() {
    var v = function arguments() { };
    expect(v.name).toBe('arguments');
  });
});
