// public method for decoding
var decode = function (input) {
  var _keyStr = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

  input = input.replace(/[^A-Za-z0-9\+\/\=]/g, "");
  var outLen = input.length * 0.75;
  if (input.charAt(input.length - 1) == "=") {
    outLen--;
  }
  if (input.charAt(input.length - 2) == "=") {
    outLen--;
  }
  var buf = new ArrayBuffer(outLen);
  var output = new Uint8Array(buf);

  var chr1, chr2, chr3;
  var enc1, enc2, enc3, enc4;
  var i = 0;
  var j = 0;


  while (i < input.length) {
    enc1 = _keyStr.indexOf(input.charAt(i++));
    enc2 = _keyStr.indexOf(input.charAt(i++));
    enc3 = _keyStr.indexOf(input.charAt(i++));
    enc4 = _keyStr.indexOf(input.charAt(i++));

    chr1 = (enc1 << 2) | (enc2 >> 4);
    chr2 = ((enc2 & 15) << 4) | (enc3 >> 2);
    chr3 = ((enc3 & 3) << 6) | enc4;

    output[j++] = chr1;

    if (enc3 != 64) {
      output[j++] = chr2;
    }
    if (enc4 != 64) {
      output[j++] = chr3;
    }
  }

  return buf;
}


var TeneralClient = function(path) {
  that = {};
  that.w = new WebSocket(path);

  that.reqId=0;

  that.procs = {};

  that.execute = function(cmd, args, reqId) {
    var msg = { "execute" : 
                { "command"   : cmd,
                  "arguments" : args,
                  "requestId" : reqId }};
    that.w.send(JSON.stringify(msg));
  }

  that.w.onerror = function(m) { console.log(m); };
  that.w.onmessage = function(m) {
    fr = new FileReader();
    fr.onloadend = function (evt) {
      try {
        var obj = jQuery.parseJSON(evt.target.result);
        if (obj.newProcess) {
          // Create a new command window.
          var pid = obj.newProcess.pid;
          $("<div class=\"tnrl-process\" id=\"tnrl-process-" + obj.newProcess.pid + "\"><span class=\"tnrl-title\"></span><span class=\"tnrl-min\">-</span><span class=\"tnrl-close\">x</span><div class=\"tnrl-content\"><div class=\"tnrl-stdout\" tab-index=\"0\"></div><div class=\"tnrl-stderr\"></div></div></div>").prependTo("#tnrl-content");
          var procWin  = $("#tnrl-process-" + pid);
          var proc  = $("#tnrl-process-" + pid + " .tnrl-content");
          var close = $("#tnrl-process-" + pid + " > .tnrl-close");
          var min = $("#tnrl-process-" + pid + " > .tnrl-min");
          var procContent = $("#tnrl-process-" + pid + " .tnrl-stdout");
          $("#tnrl-process-" + pid + " > .tnrl-title").html(obj.newProcess.command);

          close.ready( function() { close.click(function (ev) {
            procWin.slideUp("fast");
          })});

          $("#tnrl-process-" + pid + " > .tnrl-title").html(obj.newProcess.command);

          var maxFunc;
          var minFunc = function (ev) {
            proc.slideUp("fast");
            min.html("+");
            min.unbind("click");
            min.click(maxFunc);
          };

          maxFunc = function(ev) {
            proc.slideDown("fast");
            min.html("-");
            min.unbind("click");
            min.click(minFunc);
          };

          min.ready(min.click(minFunc));
          proc.ready(function () {
            proc.resizable({handles: 's'});
          });

          that.procs[pid] = {};
          that.procs[pid].output = "";
          var cmd = obj.newProcess.command;

          // HACK: If the command ended in .jpg, interpret its output as JPG.
          if(cmd.substr(cmd.length-4, cmd.length) == ".jpg") {
            that.procs[pid].type = "jpeg";
          }
        } else if (obj.processOutput) {
          var pid = obj.processOutput.pid;
          var stdout = $("#tnrl-process-" + pid + " .tnrl-stdout");
          var div = $("#tnrl-process-" + pid + " .tnrl-content");

          if (that.procs[pid].type == "jpeg") {
            var blob = new Blob(
                [that.procs[pid].output, decode(obj.processOutput.data)]);
            that.procs[pid].output = blob;
          } else {
            var blob = new Blob([decode(obj.processOutput.data)]);
            var reader = new FileReader();
            reader.onload = function(f) {
              stdout.append(document.createTextNode(f.target.result));
              div = $("#tnrl-process-" + pid + " .tnrl-content");
              div.scrollTop(div.prop("scrollHeight")+20);
              // TODO:  Good height handling.
              div.resizable({handles: 's' });
            }
            reader.readAsText(blob);
          }
        } else if (obj.processTerminated) {
          var pid = obj.processTerminated.pid;

          // If it was a JPG command, render it via data-URI.
          if (that.procs[pid].type == "jpeg") {
            var stdout = $("#tnrl-process-" + pid + " .tnrl-stdout");
            var url = window.webkitURL.createObjectURL(that.procs[pid].output);
            stdout.html("<img src=\"" + url + "\"/>");
          } 

          // Set the border color based on the return code.
          var ret = obj.processTerminated.returnCode;
          if (ret == 0) {
            $("#tnrl-process-" + pid).addClass("process-complete", "slow");
          } else {
            $("#tnrl-process-" + pid).addClass("process-failed", "slow");
          }
        }
      } catch (err) {
        // Error!
      }
    };
    fr.onerror = function (e) { console.log(e);};
    fr.readAsText(m.data, "ASCII");

  };

  // Welcome message, executed on the server to prove we have connectivity:
  that.w.onopen = function () {
    that.execute("cowsay Welcome to tnrl.", [["cowsay", "Welcome", "to", "tnrl."]], that.reqId);
  };

  return that;
};

var t = new TeneralClient("ws://localhost:10002/ws");

// Hook our prompt to w.execute via a simple parser.
$("#tnrl-prompt").ready(function() {
  $("#tnrl-prompt").keydown(function(event) {
    if(event.keyCode == 13) {
      var cmd_str = $("#tnrl-prompt").val();
      var cmd = cmd_str.split(" ");

      var cmds = [[]];
      var cmdNum = 0;
      for (var i = 0; i != cmd.length; i++) {
        var c = cmd[i];
        if( c == "|" ) {
          cmdNum++;
          cmds[cmdNum] = []
          continue;
        }
        cmds[cmdNum].push(c);
      }
      
      t.execute(cmd_str, cmds, t.reqId);
      $("#tnrl-prompt").val("");
    }
  });
});

