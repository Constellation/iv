describe("Math", function() {
  it("asinh", function() {
    expect(isNaN(Math.asinh(NaN))).toBe(true);
    expect(Math.asinh(0)).toBe(0);
    expect(1 / Math.asinh(0)).toBe(Infinity);
    expect(Math.asinh(-0)).toBe(-0);
    expect(1 / Math.asinh(-0)).toBe(-Infinity);
    expect(Math.asinh(Infinity)).toBe(Infinity);
    expect(Math.asinh(-Infinity)).toBe(-Infinity);
  });
});
