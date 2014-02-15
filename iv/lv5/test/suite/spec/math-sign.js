describe("Math", function() {
  it('sign', function() {
    expect(isNaN(Math.sign(NaN))).toBe(true);
    expect(Math.sign(-0)).toBe(-0);
    expect(1 / Math.sign(-0)).toBe(-Infinity);
    expect(Math.sign(0)).toBe(0);
    expect(1 / Math.sign(0)).toBe(Infinity);
    expect(Math.sign(-20)).toBe(-1);
    expect(Math.sign(20)).toBe(1);
  });
});
