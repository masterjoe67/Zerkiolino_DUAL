library IEEE;
use IEEE.std_logic_1164.all;

entity vga_xbus_gateway is
  port (
    clk           : in  std_logic;
    rst_n         : in  std_logic;
    xbus_addr_i   : in  std_logic_vector(31 downto 0); 
    xbus_data_i   : in  std_logic_vector(31 downto 0);
    xbus_data_o   : out std_logic_vector(31 downto 0);
    xbus_we_i     : in  std_logic;
    xbus_st_i     : in  std_logic; 
    xbus_cy_i     : in  std_logic; 
    xbus_ack_o    : out std_logic;
    
    -- Interfaccia verso il sottosistema (Nuova configurazione)
    vga_data_o    : out std_logic_vector(15 downto 0);
    vga_cmd_o     : out std_logic_vector(2 downto 0);
    vga_st_o      : out std_logic; -- STROBE PIXEL (Indirizzo 110)
    vga_st_cmd_o  : out std_logic; -- STROBE COMANDO (Indirizzi 001-101)
    vga_busy_i    : in  std_logic
  );
end entity;

architecture rtl of vga_xbus_gateway is
    signal shot_fired : std_logic := '0';
    signal ack_q      : std_logic := '0';
begin
    -- Risposta del bus
    xbus_ack_o <= ack_q;
    
    -- Lettura dello stato Busy
    xbus_data_o <= (0 => vga_busy_i, others => '0');

    process(clk, rst_n)
        variable current_cmd : std_logic_vector(2 downto 0);
    begin
        if rst_n = '0' then
            vga_st_o     <= '0';
            vga_st_cmd_o <= '0';
            shot_fired   <= '0';
            ack_q        <= '0';
            vga_data_o   <= (others => '0');
            vga_cmd_o    <= (others => '0');
        elsif rising_edge(clk) then
            -- Reset automatico degli strobi (attivi per un solo ciclo di clk_cpu)
            vga_st_o     <= '0';
            vga_st_cmd_o <= '0';
            
            -- Gestione ACK (Handshake standard)
            ack_q <= (xbus_st_i and xbus_cy_i) and (not ack_q);

            if (xbus_st_i = '1' and xbus_cy_i = '1') then
                if xbus_we_i = '1' then
                    -- Presentiamo i dati e il comando al subsystem
                    vga_data_o <= xbus_data_i(15 downto 0);
                    current_cmd := xbus_addr_i(4 downto 2);
                    vga_cmd_o  <= current_cmd;

                    -- Logica di discriminazione dello Strobe
                    if shot_fired = '0' then
                        if current_cmd = "110" then
							vga_st_o <= '1'; -- 0x18 resta lo STROBE PIXEL
						elsif current_cmd = "111" then
							vga_st_cmd_o <= '1'; -- 0x1C diventa il nuovo REG_MODE (Strobe comando)
						else
							vga_st_cmd_o <= '1'; -- Gli altri 0x04-0x14
						end if;
                        shot_fired <= '1';
                    end if;
                end if;
            else
                -- Fine del ciclo bus: pronti per il prossimo comando
                shot_fired <= '0';
                ack_q      <= '0';
            end if;
        end if;
    end process;
end architecture;