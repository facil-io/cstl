
<script language='javascript' id='header_folder'>

var folding_containers = [];
function unfold(e) {
  for (var i = 0; i < folding_containers.length; i++) {
    folding_containers[i].style.height = "0";
  }
  document.getElementById(e.target.id + "_container").style.height = "";
}

function move2containers() {
  var root = document.getElementById("header_folder").parentNode;
  var children = root.childNodes;
  var headers = [];
  var tmp = false;
  for (var i = 0; i < children.length; i++) {
    if (children[i].nodeName == 'H2') {
    	console.log(children[i]);
      tmp = [];
      tmp.push(children[i]);
      headers.push(tmp);
      continue;
    }
    if (tmp) {
      tmp.push(children[i]);
    }
  }
  while (headers.length) {
    tmp = headers.pop();
  	if(tmp.length < 2)
  		continue;
    console.log(tmp[0], `has ${tmp.length - 1} children`);
    var c = document.createElement("div");
    c.id = tmp[0].id + "_container";
    c.style.transition = "height 0.35s ease-in-out";
    // c.style.overflowY = "hidden";
    c.style.height = "0";
    folding_containers.push(c);
    root.insertBefore(c, tmp[1])
    for (var i = 1; i < tmp.length; i++) {
    	c.appendChild(tmp[i]);
    }
    tmp[0].onclick = unfold;
  }
}

// move2containers();

</script>
