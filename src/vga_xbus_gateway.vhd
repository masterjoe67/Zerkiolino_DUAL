library IEEE;
use IEEE.std_logic_1164.all;

entity vga_xbus_gateway is
  port (
    clk           : in  std_logic; -- clk_cpu (140MHz)
    rst_n         : in  std_logic;
    xbus_addr_i   : in  std_logic_vector(31 downto 0); 
    xbus_data_i   : in  std_logic_vector(31 downto 0);
    xbus_data_o   : out std_logic_vector(31 downto 0);
    xbus_we_i     : in  std_logic;
    xbus_st_i     : in  std_logic; 
    xbus_cy_i     : in  std_logic; 
    xbus_ack_o    : out std_logic;
    
    -- Interfaccia verso il sottosistema (Aggiornata a 4 bit)
    vga_data_o    : out std_logic_vector(15 downto 0);
    vga_cmd_o     : out std_logic_vector(3 downto 0); -- AMPLIATO
    vga_st_o      : out std_logic; -- STROBE PIXEL
    vga_st_cmd_o  : out std_logic; -- STROBE COMANDO
    vga_busy_i    : in  std_logic
  );
end entity;

architecture rtl of vga_xbus_gateway is
    signal ack_q : std_logic := '0';
begin
    xbus_ack_o <= ack_q;
    xbus_data_o <= (0 => vga_busy_i, others => '0');

    process(clk, rst_n)
        variable current_cmd : std_logic_vector(3 downto 0);
    begin
        if rst_n = '0' then
            vga_st_o     <= '0';
            vga_st_cmd_o <= '0';
            ack_q        <= '0';
            vga_data_o   <= (others => '0');
            vga_cmd_o    <= (others => '0');
        elsif rising_edge(clk) then
            -- Reset automatico degli strobi (durano 1 solo clock)
            vga_st_o     <= '0';
            vga_st_cmd_o <= '0';

            -- Logica di Handshake Wishbone pulita
            if (xbus_st_i = '1' and xbus_cy_i = '1') then
                if ack_q = '0' then
                    -- Primo ciclo del trasferimento: eseguiamo l'azione
                    ack_q <= '1'; 
                    if xbus_we_i = '1' then
                        vga_data_o <= xbus_data_i(15 downto 0);
                        current_cmd := xbus_addr_i(5 downto 2);
                        vga_cmd_o  <= current_cmd;

                        -- Distribuzione Strobi
                        if current_cmd = "0110" then    -- 0x18 (Pixel)
                            vga_st_o <= '1';
                        else
                            vga_st_cmd_o <= '1';        -- Tutti gli altri (incluso Scroll 0x9)
                        end if;
                    end if;
                end if;
                -- Se ack_q è già 1, resta a 1 finché xbus_st_i non scende (Wait state)
            else
                ack_q <= '0';
            end if;
        end if;
    end process;
end architecture;