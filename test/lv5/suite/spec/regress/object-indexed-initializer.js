describe("Object", function() {
  it("indexed initializer", function() {
    var obj = {
      0: 0,
      1: 42
    };
    expect(obj[0]).toBe(0);
    expect(obj[1]).toBe(42);

    for (var i = 0; i < 1; ++i) {
      expect(obj[i]).toBe(0);
    }

    for (var i = 1; i < 2; ++i) {
      expect(obj[i]).toBe(42);
    }
  });
});
