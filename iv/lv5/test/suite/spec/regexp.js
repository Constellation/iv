describe("RegExp", function() {
  it("replace with 3 digits delimiter", function() {
    expect('1211'.replace(/\B(?=(?:\d{3})+$)/g, ',')).toBe('1,211');
  });
});
