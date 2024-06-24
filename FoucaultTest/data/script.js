
$(document).ready(function(){
    $("#btn2").click( function(){
            $.post("btnActive", {
                stateBtnActive : 1,
            })
    });

    $("#btnStop").click( function(){
        $.post("btnStop", {
            btnStop : 1,
        })
    });

});

$(document).ready(function(){
    $("#btn1").click( function(){
     document.getElementById('btn1').style.display = 'none'; 
     document.getElementById('btn2').style.display = ''; 
    });
});


function ActionButton()
{
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "push", true);
    xhttp.send();

    // document.getElementById('btn1').style.display = 'none'; 
    // document.getElementById('btn2').style.display = ''; 
}

function BlockButton() {
    var btn = document.getElementById('btn2');
    btn.disabled = true; 
    setTimeout(function() {
      btn.disabled = false; 
    }, 1000); 
  }
  

  


// function Config()
// {
//     var xhr = new XMLHttpRequest();
//     xhr.open("GET", "sendInfoClick", true); 
//     var data = "click";
//     xhr.send(data);
    
// }




// setInterval(function getData()
// {
//     var xhttp = new XMLHttpRequest();

//     xhttp.onreadystatechange = function()
//     {
//         if(this.readyState == 4 && this.status == 200)
//         {
//             if(this.responseText == 0){
//                 document.getElementById("btn3").disabled = false;
//             }else if(this.responseText == 1){
//                 document.getElementById("btn3").disabled = true;

//             }else if (this.responseText == null){

//             }
            
            
//         }
//     };

//     xhttp.open("GET", "readBtn", true);
//     xhttp.send();
// }, 5000);





// document.addEventListener("DOMContentLoaded", function() {
//     var element = document.querySelector("#click");
//     if (element) {
//         element.addEventListener("click", function() {
//             var xhr = new XMLHttpRequest();
//             xhr.open("POST", "/sendInfo", true); 
//             var data = "data"; 
//              xhr.send(data);
//         });
//     }
// });


/*
setInterval(fonction getData(),
{
    var xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = fonction();
    {
        if(this.readystate == 4 && this.status == 200)
        {
            document.getElementById("VitessePorte").innerHTML = this.responseText;
        }
    };
    xhttp.open("GET", "ReadVitessePorte", true);
    xhttp.send();
} 2000);

function TotalClick(click)
{
    var VitessePorte = document.getElementById("VitessePorte");
    var value = parseInt(VitessePorte.innerText) + click;
    console.log(value);
    console.log(document.getElementById("VitessePorte").innerHTML);

    if(value < 1)
    {
        value = 1 ; 
    }

    if(value > 5)
    {
        value = 5 ; 
    }

    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "SpeedChange?value=" + value, true);
    xhttp.send();
    document.getElementById("VitessePorte").innerText = value;
}

*/
