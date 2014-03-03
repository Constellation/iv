describe("arithmetic", function() {
  function divide(f0, f1) {
    return f0 / f1;
  }
  it("1 / -0", function() {
    expect(divide(1, -0.0)).toBe(Number.NEGATIVE_INFINITY);
    expect(1 / (-0 / 1)).toBe(Number.NEGATIVE_INFINITY);
    expect(1 / (+0 / -Number.MAX_VALUE)).toBe(Number.NEGATIVE_INFINITY);
    expect(1 / (-0 / Number.MIN_VALUE)).toBe(Number.NEGATIVE_INFINITY);
    expect(1 / (1 / Number.NEGATIVE_INFINITY)).toBe(Number.NEGATIVE_INFINITY);
  });

  it("1 / 0", function() {
    expect(divide(1,  0.0)).toBe(Number.POSITIVE_INFINITY);
  });
});
