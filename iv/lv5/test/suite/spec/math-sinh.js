describe("Math", function() {
  it("sinh", function() {
    expect(isNaN(Math.sinh(NaN))).toBe(true);
    expect(Math.sinh(0)).toBe(0);
    expect(1 / Math.sinh(0)).toBe(Infinity);
    expect(Math.sinh(-0)).toBe(-0);
    expect(1 / Math.sinh(-0)).toBe(-Infinity);
    expect(Math.sinh(Infinity)).toBe(Infinity);
    expect(Math.sinh(-Infinity)).toBe(-Infinity);
  });
});
