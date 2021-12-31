# Trabajo Práctico cuatrimestral asignatura Sistemas Operativos.

Consiste en el desarrollo de una solución que permita la simulación de un sistema distribuido, donde se tendrán que planificar procesos externos, que ejecuten peticiones al sistema de recursos. El sistema deberá, mediante esta interacción, habilitar recursos de memoria (bajo un esquema de paginación pura), recursos de entrada-salida y semáforos.

## [Consigna del Trabajo Práctico](https://docs.google.com/document/d/1BDpr5lfzOAqmOOgcAVg6rUqvMPUfCpMSz1u1J_Vjtac/edit)

# Compilación de Modulo 💿

## Build/Debug de un modulo 💻

## Scripts
> Con el objetivo de que el `build` y el `debug` seán más simples para el equipo se crearon varios scripts **bash** con el sufijo `.sh` en donde los mismos:
* Haran un `MAKE` al modulo en donde se encuentre y además correrán la aplicación con las determinadas herramientas de la catedra. Sean **Valgrind** o **Helgrind**.
* Generarán la carpeta `obj/` que se que contendrán los objetos generados en el make.
* Generarán la carpeta `obj/` en la carpeta `shared/` que se que contendrán los objetos de los diferentes archivos ubicados en la carpeta mencionada.

> Actualmente se poseen tres scripts generados,todos tienen como objetivo realizar el make del modulo, los mismos tienen como objetivo:
- `exec.sh`  Ejecuta el modulo.
- `vexec.sh` Ejecuta el modulo utilizando `valgrind`
- `hexec.sh` Ejecuta el modulo utilizando `helgrind`

Los mismos se correr con el comando `sh` seguido del nombre del script que se desee utilizar.

### Ejemplo de uso:
```
utnso@ubuntu-server:~/tp-2021-2c-3era-es-la-vencida/kernel$ sh exec.sh
```

## Tasks
> Adicionalmente también contamos con task de *JavaScript* que pueden ser utilizadas desde ***VsCode***
Para utilizar las mismas deberá ingresar a `terminal -> run task` y seleccionar la task que desee ejecutar.

**IMPORTANTE:** Las task cumplen la misma función que los scripts, realizan lo mismo pero sin la necesidad de utilizar la consola.

## Debug
Para realizar el debug de la aplicación se deberá ingresar a la sección de *Debug* de **VsCode** y en la misma aparecerá su modulo a debuggear.

> Si necesita ayuda con el Debug o no sabe como debuggear desde VsCode. Recomiendo que visite [VsCode Debug](https://code.visualstudio.com/docs/editor/debugging)
  
