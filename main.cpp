#include <aegis.hpp>
#include <Windows.h>

#include "sticker_addon.h"
#include "tools.h"
#include "guild_here.h"
#include "user_here.h"
#include "delay_tasker.h"
#include "chat_memory.h"
#include "keys.h"

bool up = true;

/*
Falta:
- verificar roles
- chat config (in memory) de qual foi a última vez que alguém falou no chat X e quem foi
*/

const aegis::snowflake meedev           = 280450898885607425;

const std::string default_cmd           = u8"lsw/sl";
const std::string version_app           = u8"V1.0.1";

const auto emoji_yes                    = u8"✅";
//const std::string emoji_no              = easy_simple_emoji(u8"❎");

const int range_low = -10;
const int range_high = 35;
const int range_total = range_high - range_low; // easier

const int range_boost_low = 40;
const int range_boost_high = 400;
const int range_boost_total = range_boost_high - range_boost_low; // easier

const unsigned range_boost_chances = 42; // one in 42

const std::string generic_help =
    u8"`" + default_cmd + u8" ?`: mostra isso\n" // done
    u8"`" + default_cmd + u8" help`: mostra isso\n" // done
    u8"`" + default_cmd + u8" ajuda`: mostra isso\n" // done
    u8"`" + default_cmd + u8" upgrade <vezes>`: tenta comprar o(s) próximo(s) cargos registrado nesse grupo (gasta pontos globais, máximo 10 de uma vez)\n" // done
    u8"`" + default_cmd + u8" info`: mostra informações sobre este grupo e dados seus.\n\n" // done
    u8"*ESTE BOT ESTÁ EM BETA. Se você acha que tem algum bug, por favor, reporte a um admin.*\n"; // done

const std::string admin_help = u8"**Comandos de admin:**\n"
    u8"`" + default_cmd + u8" reg <num> <id>`: registrar que o usuário pode comprar um cargo `<id>` com uma quantidade de pontos\n" // done
    u8"`" + default_cmd + u8" unreg <id|*>`: tirar configuração de compra de cargo `<id>` ou todos de uma vez com `*`\n" // done
    u8"`" + default_cmd + u8" list`: lista na ordem que o usuário deve obter (na ordem que foi adicionado) os cargos e o custo em pontos\n" // done
    u8"`" + default_cmd + u8" retry <id>`: tenta aplicar todos os cargos que o usuário com ID `<id>` devia ter no grupo.\n" // done
    u8"`" + default_cmd + u8" log <id>`: faz uma cópia vitalícia dos pontos que as pessoas ganharam no seu grupo aqui.\n"
    u8"\n**Comandos gerais:**\n"; // done

// EMBED STUFF
const int color                         = 0x7554ED;
const std::string url_points            = "https://cdn.discordapp.com/attachments/739704685505544363/740364166769934407/stonks_dna.png";

using namespace LSW::v5::Tools;


int main(int argc, char* argv[])
{
    custom_random random;

    DelayedTasker tasker;

    //std::vector<Guild> gds;
    std::mutex gds_m;

    try
    {
        // Create bot object
        aegis::core bot(aegis::create_bot_t().log_level(spdlog::level::trace).token(key));

        bot.set_on_message_create_raw([&](nlohmann::json j, aegis::shards::shard* _shard) {
            if (!up) return;

            auto& data = j["d"];

            if (!data.count("author") || data["author"].is_null()) return; // non user

            auto& author = data["author"]; // shortcut

            if (author.count("bot") && author["bot"].is_boolean() && author["bot"].get<bool>()) return; // bot
            if ((author.count("discriminator") && !author["discriminator"].is_string())) return; // webhook

            const aegis::snowflake who = author["id"]; // whoever has sent
            const std::string who_avatar = (!author.count("avatar") || !author["avatar"].is_string()) ? "" : "https://cdn.discordapp.com/avatars/" + std::to_string(who) + "/" + author["avatar"].get<std::string>() + ".png?size=256"; // author["id"]; // whoever has sent
            const std::string who_str = author["username"].get<std::string>() + "#" + author["discriminator"].get<std::string>();
            const std::string content = data["content"]; // find commands
            const aegis::snowflake guild = data["guild_id"]; // guild ownership control
            const aegis::snowflake channel = data["channel_id"]; // response
            const aegis::snowflake message = data["id"]; // for reactions

            aegis::channel* ch_a = bot.find_channel(channel);
            if (!ch_a) {
                bot.log->error("Cannot find what channel event came from in guild {}", guild);
                return;
            }


            aegis::guild* guild_a = bot.find_guild(guild);
            if (guild_a && content.find(default_cmd) == 0) {

                ch_a->get_message(message).then<void>(
                    [&](aegis::gateway::objects::message m) {
                        tasker.delay(10000, [m = std::move(m)]() mutable { m.delete_message(); });
                    }
                );

                const auto argg = content.substr(default_cmd.length());
                const bool is_god = guild_a->get_owner() == who || who == meedev;

                if (argg.find(" restart") == 0 && who == meedev)
                {
                    ch_a->create_reaction(message, emoji_yes, false);
                    bot.update_presence(default_cmd + " - restarting...", aegis::gateway::objects::activity::Game, aegis::gateway::objects::presence::Idle);
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    up = false;
                    tasker.close();
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    bot.shutdown();
                    return;
                }
                else if (argg.find(" forceset") == 0 && who == meedev)
                {
                    unsigned long long userid{}, npts{};

                    if (sscanf_s(argg.c_str(), " forceset %llu %llu", &userid, &npts) == 2)
                    {
                        auto& usr = global_users.grab_user(userid);
                        std::lock_guard<std::mutex> l(usr.m);

                        usr.pts = npts;
                        global_users.flush(userid);

                        ch_a->create_message(
                            u8"Definiu com sucesso <@" + std::to_string(userid) + u8"> para ter `" + std::to_string(usr.pts) + u8"` pontos."
                        ).then<void>(
                            [&](aegis::gateway::objects::message m) {
                                tasker.delay(10000, [m = std::move(m)]() mutable { m.delete_message(); });
                            }
                        );
                    }
                    else {
                        ch_a->create_message(
                            u8"Não pude ler com sucesso os números. Por acaso você seguiu mesmo o formato `" + default_cmd + u8" forceset <user> <pts>` como por exemplo `" + default_cmd + u8" forceset 123456789012345678 50`?"
                        ).then<void>(
                            [&](aegis::gateway::objects::message m) {
                                tasker.delay(30000, [m = std::move(m)]() mutable { m.delete_message(); });
                            }
                        );
                    }

                }
                else if (is_god && argg.find(" status") == 0)
                {
                    std::string resume = u8"**About:**\n```md\n";
                    resume += "- Delayed tasks amount: " + std::to_string(tasker.delayed_tasks_size()) + " tasks\n";
                    resume += "- Users in memory:      " + std::to_string(global_users.users_known_size()) + "\n";
                    resume += "- Memory usage:         " + std::to_string(double(aegis::utility::getCurrentRSS()) / (1u << 20)) + " MB\n";
                    resume += "- Peak memory usage:    " + std::to_string(double(aegis::utility::getPeakRSS()) / (1u << 20)) + " MB\n";
                    resume += "- Uptime:               " + bot.uptime_str() + "\n";
                    resume += "- Version:              " + version_app + "\n";
                    resume += "```";

                    ch_a->create_message(
                        resume
                    ).then<void>(
                        [&](aegis::gateway::objects::message m) {
                            tasker.delay(30000, [m = std::move(m)]() mutable { m.delete_message(); });
                        }
                    );
                }
                else if (is_god && argg.find(" reg") == 0) // reg <num> <id>
                {
                    unsigned long long pts{}, role{};

                    if (sscanf_s(argg.c_str(), " reg %llu %llu", &pts, &role) == 2) {

                        global_guilds.reg_add(guild, role, pts);

                        ch_a->create_message(
                            u8"Registrado: `" + std::to_string(pts) + u8"` pontos para o cargo <@&" + std::to_string(role) + u8">."
                        ).then<void>(
                            [&](aegis::gateway::objects::message m) {
                                tasker.delay(30000, [m = std::move(m)]() mutable { m.delete_message(); });
                            }
                        );
                    }
                    else {
                        ch_a->create_message(
                            u8"Não pude ler com sucesso os números. Por acaso você seguiu mesmo o formato `" + default_cmd + u8" reg <num> <id>` como por exemplo `" + default_cmd + u8" reg 20 123456789012345678`?"
                        ).then<void>(
                            [&](aegis::gateway::objects::message m) {
                                tasker.delay(30000, [m = std::move(m)]() mutable { m.delete_message(); });
                            }
                        );
                    }
                }
                else if (is_god && argg.find(" list") == 0)
                {
                    const std::string res = global_guilds.list(guild);

                    ch_a->create_message(
                        u8"Configurações existentes neste servidor:\n" + res
                    ).then<void>(
                        [&](aegis::gateway::objects::message m) {
                            tasker.delay(30000, [m = std::move(m)]() mutable { m.delete_message(); });
                        }
                    );
                }
                else if (is_god && argg.find(" unreg") == 0)
                {
                    unsigned long long role{};

                    if (argg.find(" unreg *") == 0) {

                        global_guilds.reg_remove_all(guild);

                        ch_a->create_message(
                            u8"Você tirou todas as configurações de cargo."
                        ).then<void>(
                            [&](aegis::gateway::objects::message m) {
                                tasker.delay(30000, [m = std::move(m)]() mutable { m.delete_message(); });
                            }
                        );
                    }
                    else if (sscanf_s(argg.c_str(), " unreg %llu", &role) == 1) {

                        global_guilds.reg_remove(guild, role);

                        ch_a->create_message(
                            u8"Você tirou do registro cargo <@&" + std::to_string(role) + u8">."
                        ).then<void>(
                            [&](aegis::gateway::objects::message m) {
                                tasker.delay(30000, [m = std::move(m)]() mutable { m.delete_message(); });
                            }
                        );
                    }
                    else {
                        ch_a->create_message(
                            u8"Não pude ler com sucesso o número. Por acaso você seguiu mesmo o formato `" + default_cmd + u8" unreg <id>` como por exemplo `" + default_cmd + u8" unreg 123456789012345678`?"
                        ).then<void>(
                            [&](aegis::gateway::objects::message m) {
                                tasker.delay(30000, [m = std::move(m)]() mutable { m.delete_message(); });
                            }
                        );
                    }
                }
                else if (is_god && argg.find(" retry") == 0)
                {
                    unsigned long long usrid{};

                    if (sscanf_s(argg.c_str(), " retry %llu", &usrid) == 1) {

                        global_guilds.reapply_user_on_guild(*guild_a, guild, usrid, global_users.grab_user(usrid));

                        ch_a->create_message(
                            u8"Se há permissão para eu definir cargos, o usuário deve estar com todos os cargos que comprou (em pouco tempo)."
                        ).then<void>(
                            [&](aegis::gateway::objects::message m) {
                                tasker.delay(30000, [m = std::move(m)]() mutable { m.delete_message(); });
                            }
                        );
                    }
                    else {
                        ch_a->create_message(
                            u8"Não pude ler com sucesso o número. Por acaso você seguiu mesmo o formato `" + default_cmd + u8" retry <id>` como por exemplo `" + default_cmd + u8" retry 123456789012345678`?"
                        ).then<void>(
                            [&](aegis::gateway::objects::message m) {
                                tasker.delay(30000, [m = std::move(m)]() mutable { m.delete_message(); });
                            }
                        );
                    }
                }
                else if (is_god && argg.find(" log") == 0)
                {
                    unsigned long long chatid{};
                    if (sscanf_s(argg.c_str(), " log %llu", &chatid) == 1) {

                        global_guilds.set_registry_chat(guild, chatid);

                        ch_a->create_message(
                            chatid ? 
                            u8"Você definiu o chat de log como <#" + std::to_string(chatid) + u8">." :
                            u8"Você desativou o chat de log."
                        ).then<void>(
                            [&](aegis::gateway::objects::message m) {
                                tasker.delay(30000, [m = std::move(m)]() mutable { m.delete_message(); });
                            }
                        );
                    }
                    else {
                        ch_a->create_message(
                            u8"Não pude ler com sucesso o número. Por acaso você seguiu mesmo o formato `" + default_cmd + u8" log <id>` como por exemplo `" + default_cmd + u8" log 123456789012345678`?"
                        ).then<void>(
                            [&](aegis::gateway::objects::message m) {
                                tasker.delay(30000, [m = std::move(m)]() mutable { m.delete_message(); });
                            }
                        );
                    }
                }
                else if (argg.find(" info") == 0)
                {
                    //const std::string res = global_guilds.list(guild);
                    const auto& user = global_users.grab_user(who);
                    auto next = global_guilds.next_role_for(guild, global_users.grab_user(who)); // first == role, second == points
                    const auto last_role = global_guilds.last_role_from(guild, global_users.grab_user(who));

                    const int perc = next.second > user.pts ? ((100 * user.pts) / next.second) : 100;
                    const std::string url_img = url_perc_for(perc);

                    ch_a->create_message_embed({}, {
                        { "author",
                            {
                                { "name", who_str },
                                { "icon_url", who_avatar }
                            }
                        },
                        { "title", u8"**Informações sobre você:**" },
                        { "description", u8"A barra de progresso indica quanto avançou no nível atual" },
                        { "color", color },
                        {
                            "fields", {
                                { {"name", u8"Último cargo comprado:" }, { "value", (last_role ? (u8"<@&" + std::to_string(last_role) + u8">") : u8"*nenhum*") },{ "inline", false } },
                                { {"name", u8"Próximo cargo comprável:" }, { "value", (next.first ? u8"<@&" + std::to_string(next.first) + u8">" : u8"*você já está com o maior cargo comprável*") },{ "inline", false } },
                                { {"name", u8"Pontos:" }, { "value", u8"`" + std::to_string(global_users.get(who)) + u8"`" },{ "inline", false } },
                                { {"name", u8"Pode comprar próximo nível em:" }, { "value", (next.second > user.pts ? u8"`" + std::to_string(next.second - user.pts) + u8" ponto(s)`" : (next.first ? u8"*agora* `(" + default_cmd + " upgrade)`" : u8"*você já está no topo*")) },{ "inline", false } }
                            }
                        },                        
                        { "image",
                            {
                                { "url", url_img }
                            }
                        }
                    }).then<void>(
                        [&](aegis::gateway::objects::message m) {
                            tasker.delay(20000, [m = std::move(m)]() mutable { m.delete_message(); });
                        }
                    );
                }
                else if (argg.find(" upgrade") == 0)
                {
                    std::string message_buf;

                    auto try_upgrade_someone = [&](const unsigned long long whoever) {
                        auto& user = global_users.grab_user(whoever);
                        auto next = global_guilds.next_role_for(guild, user);

                        if (next.first == 0) return false;

                        if (next.second <= user.pts) {
                            {
                                std::lock_guard<std::mutex> l(user.m);
                                guild_a->add_guild_member_role(who, next.first);
                                user.conquered_roles.push_back(next.first);
                                user.pts -= next.second;
                            }
                            global_users.flush(who);

                            message_buf += "Adiquiriu: <@&" + std::to_string(next.first) + ">!\n";
                        }
                        else { // can't buy

                            if (message_buf.empty()) {
                                message_buf += u8"Futuramente você poderá comprar <@&" + std::to_string(next.first) + u8">, mas no momento você não tem pontos suficientes!\n";
                            }

                            return false;
                        }

                        return true;
                    };

                    auto& user = global_users.grab_user(who);
                    auto next = global_guilds.next_role_for(guild, user);

                    unsigned amount_of_upgrade_tries = 0;

                    if (sscanf_s(argg.c_str(), " upgrade %u", &amount_of_upgrade_tries) != 1) amount_of_upgrade_tries = 1;
                    if (amount_of_upgrade_tries > 10) amount_of_upgrade_tries = 10;

                    for (unsigned p = 0; p < amount_of_upgrade_tries; p++) {
                        if (!try_upgrade_someone(who)) break;
                    }

                    if (message_buf.empty()) message_buf = u8"Parece que não há nada para comprar pela frente.";
                    ch_a->create_message_embed({}, {
                        { "author",
                            {
                                { "name", who_str },
                                { "icon_url", who_avatar }
                            }
                        },
                        { "color", color },
                        { "description",
                            message_buf + 
                            u8"\nSeu saldo de pontos atual: `" + std::to_string(user.pts) + u8" ponto(s)`\n"
                        },
                        { "thumbnail",
                            {
                                { "url", url_points }
                            }  
                        }
                    }).then<void>(
                        [&](aegis::gateway::objects::message m) {
                            tasker.delay(20000, [m = std::move(m)]() mutable { m.delete_message(); });
                        }
                    );
                }
                else
                {
                    ch_a->create_message(
                        (is_god ? admin_help : "") + generic_help
                    ).then<void>(
                        [&](aegis::gateway::objects::message m) {
                            tasker.delay(30000, [m = std::move(m)]() mutable { m.delete_message(); });
                        }
                    );
                }
            }

            else { // not a command

                // points

                if (global_chats.points_can(channel, who)) {

                    int boost_final = 0;
                    bool had_boost = (random.random() % range_boost_chances == 0) || global_users.get(who) == 0;

                    if (had_boost) { // epic boost
                        boost_final = static_cast<int>(random.random() % range_boost_total) + range_boost_low;
                    }
                    else {
                        boost_final = static_cast<int>(random.random() % range_total) + range_low;
                    }
                    if (!boost_final) return; // no luck lol

                    global_users.add(who, boost_final);

                    const nlohmann::json final_json = {
                        { "author",
                            {
                                { "name", who_str },
                                { "icon_url", who_avatar }
                            }
                        },
                        { "color", color },
                        { "description",
                            u8"Teve alterações:\n\n"
                            u8"`" + std::string(boost_final > 0 ? u8"🔺" : u8"🔻") + u8"` **" + std::to_string(boost_final) + u8" PONTO(S)**\n" +
                            std::string(had_boost ? u8"`[⚡ BOOST DA SORTE ⚡]`\n" : u8"")
                        },
                        { "thumbnail",
                            {
                                { "url", url_points }
                            }
                        },
                        { "timestamp", time_iso() }
                    };

                    ch_a->create_message_embed({}, final_json).then<void>(
                        [&](aegis::gateway::objects::message m) {
                            tasker.delay(20000, [m = std::move(m)]() mutable { m.delete_message(); });
                        }
                    );

                    if (auto regid = global_guilds.get_registry_chat(guild); regid)
                    {
                        aegis::channel* ch = bot.find_channel(regid);
                        if (ch) ch->create_message_embed({}, final_json);
                    }
                }
            }

        });

        // start the bot
        bot.run();

        std::this_thread::sleep_for(std::chrono::seconds(10));

        std::thread thr([&] {
            std::this_thread::sleep_for(std::chrono::seconds(5));

            while (up) {
                bot.update_presence(default_cmd + u8" - " + version_app, aegis::gateway::objects::activity::Game, aegis::gateway::objects::presence::Online);
                for (size_t cc = 0; cc < 250 && up; cc++) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    std::this_thread::yield();
                }
            }
            std::cout << "Got close signal. Ending..." << std::endl;

            bot.shutdown();
        });

        // yield thread execution to the library
        bot.yield();

        up = false;
        thr.join();
    }
    catch (const std::exception& e)
    {
        std::cout << "Error during initialization: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cout << "Error during initialization: uncaught" << std::endl;
        return 1;
    }
    return 0;
}
