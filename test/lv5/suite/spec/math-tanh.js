describe("Math", function() {
  it("tanh", function() {
    expect(isNaN(Math.tanh(NaN))).toBe(true);
    expect(Math.tanh(0)).toBe(0);
    expect(1 / Math.tanh(0)).toBe(Infinity);
    expect(Math.tanh(-0)).toBe(-0);
    expect(1 / Math.tanh(-0)).toBe(-Infinity);
    expect(Math.tanh(Infinity)).toBe(1);
    expect(Math.tanh(-Infinity)).toBe(-1);
  });
});
