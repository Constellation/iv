// Copyright 2009 the Sputnik authors.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/**
 * String.prototype.substring (start, end) returns a string value(not object)
 *
 * @path ch15/15.5/15.5.4/15.5.4.15/S15.5.4.15_A2_T2.js
 * @description start is NaN, end is Infinity
 */

describe("String", function() {
  it(".substring(NaN, Infinity)", function() {
    var __string = new String('this is a string object');
    expect(__string.substring(NaN, Infinity)).toBe("this is a string object");
  });
});
