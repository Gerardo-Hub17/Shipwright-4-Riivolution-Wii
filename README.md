![Ship of Harkinian](docs/shiptitle.darkmode.png#gh-dark-mode-only)
![Ship of Harkinian](docs/shiptitle.lightmode.png#gh-light-mode-only)


# MOTOR PORTEADO A NINTENDO WII, GRACIAS AL EQUIPO ORIGINAL DE SHIP OF HARKINIAN POR EL TRABAJO

## Website
Official Website: https://www.shipofharkinian.com/

### Si vas a descargar directamente el motor solo entra acá. [Lanzamientos](https://github.com/HarbourMasters/Shipwright/releases)



# Antes de empezar
Este sitio no incluye ningún recurso protegido por derechos de autor. Es necesario que proporciones una copia compatible del juego.

# Verifica tu Rom.
Puedes verificar que has extraído una copia compatible del juego utilizando el comprobador en https://ship.equipment/. Si prefiere validar manualmente su ROM Dumpeada, puede comprobar su hash `sha1` con los hashes. [Aquí](docs/supportedHashes.json).

# Descripción general del proyecto
Ship of Harkinian (SOH) está construido sobre una biblioteca personalizada llamada libultraship (LUS). En los tiempos de la N64, había un SDK distribuido a los desarrolladores llamado libultra; LUS está diseñado para imitar la función
Para que el juego funcione, necesitarás una ROM de Ocarina of Time **adquirida legalmente**. 

### Backend Gráficos 
Se empleará el renderizado nativo de GX, dicho renderizado ya tengo escrito y implementare en el motor

# Archivos personalizados 
Aún no existe soporte*

# Desarrollo
### Building

Si desea compilar SoH manualmente, consulte [Instrucciones de compilación](docs/BUILDING.md).

### Pruebas de juego
Si quieres probar una compilación de integración continua, puedes encontrarla en los enlaces que aparecen a continuación. Ten en cuenta que estas compilaciones son solo para pruebas y es probable que encuentres errores y, posiblemente, fallos del sistema.

* [Windows](https://nightly.link/HarbourMasters/Shipwright/workflows/generate-builds/develop/soh-windows.zip)
* [macOS](https://nightly.link/HarbourMasters/Shipwright/workflows/generate-builds/develop/soh-mac.zip)
* [Linux](https://nightly.link/HarbourMasters/Shipwright/workflows/generate-builds/develop/soh-linux.zip)

### Reading
Encontrará documentación más detallada en el directorio 'docs', incluyendo la mencionada anteriormente. [building instructions](docs/BUILDING.md).

* [Credits](docs/CREDITS.md)
* [Custom Music](docs/CUSTOM_MUSIC.md)
* [Formatting](docs/FORMATTING.md)
* [Controller Mapping](docs/GAME_CONTROLLER_DB.md)
* [Modding](docs/MODDING.md)
* [Versioning](docs/VERSIONING.md)

<a href="https://github.com/Kenix3/libultraship/">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="./docs/poweredbylus.darkmode.png">
    <img alt="Powered by libultraship" src="./docs/poweredbylus.lightmode.png">
  </picture>
</a>
