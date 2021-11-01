// para dejar cosas que no están sirviendo pero tenerlas por las dudas para despues.
// ni se estan usando estas cosas




// serialización de cosas que ya no
int recibir_mensaje(){

    int str_len;
    char* string;
    int offset = 0;
    mate_inner_structure* estructura_interna = malloc(sizeof(mate_inner_structure));

    void* buffer = _recive_message(buffer, logger);
    memcpy(&(estructura_interna)->rafaga_anterior, buffer, sizeof(float));
    offset += sizeof(float); 
    memcpy(&estructura_interna)->estimacion_anterior, buffer, sizeof(float));
    offset += sizeof(float); 
    memcpy(&estructura_interna)->estimacion_siguiente, buffer, sizeof(float));
    offset += sizeof(float); 
    memcpy(&estructura_interna)->llegada_a_ready, buffer, sizeof(float));
    offset += sizeof(float); 
    memcpy(&estructura_interna)->prioridad, buffer, sizeof(int));
    offset += sizeof(int); 
    memcpy(&str_len, buffer + offset, sizeof(int));
    offset += sizeof(int);
    estructura_interna->estado = malloc(str_len + 1);
    memcpy(&estructura_interna)->estado, buffer + offset, str_len);
    memcpy(&str_len, buffer + offset, sizeof(int));
    offset += sizeof(int);
    estructura_interna->semaforo = malloc(str_len + 1);
    memcpy(&estructura_interna)->semaforo, buffer + offset, str_len);
    memcpy(&estructura_interna)->valor_semaforo, buffer, sizeof(int));
    offset += sizeof(int); 
    memcpy(&str_len, buffer + offset, sizeof(int));
    offset += sizeof(int);
    estructura_interna->dispositivo_io = malloc(str_len + 1);
    memcpy(&estructura_interna)->dispositivo_io, buffer + offset, str_len);
    memcpy(&estructura_interna)->size_memoria, buffer, sizeof(int));
    offset += sizeof(int); 
    memcpy(&estructura_interna)->addr_memfree, buffer, sizeof(int));
    offset += sizeof(int); 
    memcpy(&estructura_interna)->origin_memread, buffer, sizeof(int));
    offset += sizeof(int); 
    memcpy(&estructura_interna)->dest_memread, buffer, sizeof(int));
    offset += sizeof(int); 
    memcpy(&estructura_interna)->origin_memwrite, buffer, sizeof(int));
    offset += sizeof(int); 
    memcpy(&estructura_interna)->dest_memwrite, buffer, sizeof(int));
    offset += sizeof(int); 
    memcpy(&estructura_interna)->respuesta_a_carpincho, buffer, sizeof(int));
    offset += sizeof(int);


    ejecutar_funcion_switch(buffer->codigo_operacion);

}








