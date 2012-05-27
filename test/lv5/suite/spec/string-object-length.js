describe("String.length", function() {
  it("delete String.length returns false", function() {
    var s = new String('string');
    expect(delete s.length).toBe(false);
  });
});
