describe("Math", function() {
  it("atanh", function() {
    expect(isNaN(Math.atanh(NaN))).toBe(true);
    expect(isNaN(Math.atanh(-2))).toBe(true);
    expect(isNaN(Math.atanh(2))).toBe(true);
    expect(Math.atanh(-1)).toBe(-Infinity);
    expect(Math.atanh(1)).toBe(Infinity);
    expect(Math.tanh(0)).toBe(0);
    expect(1 / Math.tanh(0)).toBe(Infinity);
    expect(Math.tanh(-0)).toBe(-0);
    expect(1 / Math.tanh(-0)).toBe(-Infinity);
  });
});
