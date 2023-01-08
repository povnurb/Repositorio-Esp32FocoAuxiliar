
function ObtenerRedes() {

    xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/index.html", true);
    xhttp.send();
    alert("Volvera a condiciones normales cuando la falla de energia se restablesca o se apage");
}
function Encender(){
    window.location='/RELAY=ON';
}
function Apagar(){
    window.location='/RELAY=OFF';
    
}
