describe("error constructing", function() {
  it("error occurs in object construct", function() {
    function raise() {
      throw new Error();
    }
    try {
      var obj = {
        t: raise()
      };
    } catch (e) {
      expect(obj).toBe(undefined);
      return;
    }
    expect(true).toBe(false);
  });

  it("error occurs in array construct", function() {
    function raise() {
      throw new Error();
    }
    try {
      var ary = [ raise() ];
    } catch (e) {
      expect(ary).toBe(undefined);
      return;
    }
    expect(true).toBe(false);
  });
});
