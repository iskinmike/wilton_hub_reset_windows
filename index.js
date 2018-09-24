/*
 * Copyright 2018, alex at staticlibs.net
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

define([
    "wilton/dyload",
    "wilton/wiltoncall"
], function(dyload, wiltoncall) {
    "use strict";

    // load shared lib on init
    dyload({
        name: "wilton_hub_reset_windows"
    });
    

    return {
        main: function() {
            print("Calling native module ...");
            var resp = wiltoncall("reset_hub", {"pid": 0x0608, "vid": 0x05E3});
            print("Call response: [" + resp + "]");
        }
    };
});