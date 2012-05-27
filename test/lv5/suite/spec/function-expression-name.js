describe("function expression name", function() {
  it("instantiate variable which is the same name to function can be overriden", function() {
    var i = function test() {
      var test;
      return test;
    }
    expect(i()).toBe(undefined);
  });

  it("function expression name is immutable variable", function() {
    var i = function test() {
      test = 20;
      return test;
    };

    expect(i()).not.toBe(20);
  });
});
