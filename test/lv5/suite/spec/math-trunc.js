describe("Math", function() {
  it('trunc', function() {
    expect(Math.trunc(0.0)).toBe(0.0);
    expect(Math.trunc(-0.0)).toBe(-0.0);
    expect(Math.trunc(Infinity)).toBe( Infinity);
    expect(Math.trunc(-Infinity)).toBe( -Infinity);
    expect(Math.trunc(0.1)).toBe( 0);
    expect(Math.trunc(-0.1)).toBe( 0);
    expect(Math.trunc(1.0)).toBe( 1);
    expect(Math.trunc(-1.0)).toBe( -1);
  });
});
