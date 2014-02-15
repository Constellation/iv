describe("int32_t", function() {
  it("neg INT32_MIN to be 2147483648", function() {
    var i = -2147483648;
    expect(-i).toBe(2147483648);
  });
});
